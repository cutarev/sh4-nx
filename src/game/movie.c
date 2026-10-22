/* The movies. SILENT HILL 4.exe plays movie/m_XX.pac (MPEG-1 video, MPEG audio layer I) through a DirectShow graph
 * it builds itself (0x413880), whose renderer filter writes each frame into one of two 640x480 A8R8G8B8 textures
 * (0x412A50 locks them and hands their bits over). The movie object's methods are replaced here by an FFmpeg
 * decoder doing the same: the frame due on the clock goes into the texture the next 0x413170 call returns, and the
 * soundtrack goes to the mixer's FIFO (dsound.c). One movie plays at a time. */
#include "../runtime/rt.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

void ds_track_write(const float *lr, int frames);
void ds_track_clear(void);

static struct {
    uint32_t guest, bits[2];  /* the movie object the game holds; the textures' bits (0x4131C0) */
    AVFormatContext *fmt;
    AVCodecContext *vdec, *adec;
    int vs, as, eof, done, have, next;  /* done: the last frame was shown; have: m.frame is decoded, not shown yet;
                                          next: the texture it goes to */
    AVFrame *frame, *aframe;
    AVPacket *pkt;
    struct SwsContext *sws;
    SwrContext *swr;
    float gain;
    Uint64 t0;  /* ms, when Run was called */
} m;

static AVCodecContext *open_decoder(AVStream *st) {
    const AVCodec *c = avcodec_find_decoder(st->codecpar->codec_id);
    AVCodecContext *d = c ? avcodec_alloc_context3(c) : NULL;
    if (d && (avcodec_parameters_to_context(d, st->codecpar) < 0 || avcodec_open2(d, c, NULL) < 0))
        avcodec_free_context(&d);
    return d;
}

static void close_movie(void) {
    avcodec_free_context(&m.vdec);
    avcodec_free_context(&m.adec);
    avformat_close_input(&m.fmt);
    av_frame_free(&m.frame);
    av_frame_free(&m.aframe);
    av_packet_free(&m.pkt);
    sws_freeContext(m.sws);
    swr_free(&m.swr);
    memset(&m, 0, sizeof m);
    ds_track_clear();
}

static void decode_audio(void) {
    if (avcodec_send_packet(m.adec, m.pkt) < 0) return;
    while (avcodec_receive_frame(m.adec, m.aframe) == 0) {
        int cap = swr_get_out_samples(m.swr, m.aframe->nb_samples);
        float *lr = malloc((size_t)cap * 8);
        uint8_t *out[1] = {(uint8_t *)lr};
        int n = swr_convert(m.swr, out, cap, (const uint8_t **)m.aframe->extended_data, m.aframe->nb_samples);
        for (int i = 0; i < 2 * n; i++) lr[i] *= m.gain;
        if (n > 0) ds_track_write(lr, n);
        free(lr);
    }
}

/* Reads packets until the next video frame comes out, feeding the soundtrack on the way. */
static int decode_video(void) {
    for (;;) {
        int r = avcodec_receive_frame(m.vdec, m.frame);
        if (r == 0) return 1;
        if (r != AVERROR(EAGAIN) || m.eof) return 0;
        if (av_read_frame(m.fmt, m.pkt) < 0) {
            m.eof = 1;
            avcodec_send_packet(m.vdec, NULL);  /* drain */
            continue;
        }
        if (m.pkt->stream_index == m.vs) avcodec_send_packet(m.vdec, m.pkt);
        else if (m.pkt->stream_index == m.as && m.adec) decode_audio();
        av_packet_unref(m.pkt);
    }
}

static double frame_time(void) {  /* seconds from the movie's start */
    AVStream *st = m.fmt->streams[m.vs];
    int64_t t = m.frame->best_effort_timestamp, t0 = st->start_time;
    return t == AV_NOPTS_VALUE ? 0 : (t - (t0 == AV_NOPTS_VALUE ? 0 : t0)) * av_q2d(st->time_base);
}

static double elapsed(void) { return m.t0 ? (SDL_GetTicks64() - m.t0) / 1000.0 : 0; }

/* obj create(void) */
void sub_00413AC0(void) {
    if (m.guest) close_movie();
    g_eax = m.guest = galloc(0x30);
    memset(GPTR(m.guest), 0, 0x30);
    g_esp += 4;
}

/* HRESULT open(obj, path), stdcall: 0, or negative when it cannot be played. */
void sub_00413880(void) {
    char path[512];
    rt_path(GSTR(ARG(1)), path, sizeof path);
    g_eax = 0x80004005u;  /* E_FAIL */
    av_log_set_level(AV_LOG_ERROR);
    if (avformat_open_input(&m.fmt, path, NULL, NULL) < 0 || avformat_find_stream_info(m.fmt, NULL) < 0 ||
        (m.vs = av_find_best_stream(m.fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0 ||
        !(m.vdec = open_decoder(m.fmt->streams[m.vs]))) {
        rt_log("movie %s: FFmpeg cannot play it", path);
    } else {
        m.as = av_find_best_stream(m.fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
        if (m.as >= 0 && (m.adec = open_decoder(m.fmt->streams[m.as]))) {
            AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
            if (swr_alloc_set_opts2(&m.swr, &stereo, AV_SAMPLE_FMT_FLT, 44100, &m.adec->ch_layout,
                                    m.adec->sample_fmt, m.adec->sample_rate, 0, NULL) < 0 || swr_init(m.swr) < 0)
                avcodec_free_context(&m.adec);
        }
        m.frame = av_frame_alloc();
        m.aframe = av_frame_alloc();
        m.pkt = av_packet_alloc();
        m.gain = 1;
        ds_track_clear();
        rt_log("movie %s: %dx%d%s", path, m.vdec->width, m.vdec->height, m.adec ? " with sound" : "");
        g_eax = 0;
    }
    g_esp += 12;
}

/* void set_textures(obj, void *bits[2]) */
void sub_004131C0(void) {
    m.bits[0] = MEM32(ARG(1));
    m.bits[1] = MEM32(ARG(1) + 4);
    g_esp += 4;
}

/* void set_volume(obj, centibels): IBasicAudio::put_Volume, -10000 (silent) to 0. */
void sub_00413130(void) {
    m.gain = (float)pow(10.0, (int32_t)ARG(1) / 2000.0);
    g_esp += 4;
}

/* HRESULT run(obj) / stop(obj), stdcall */
void sub_00413090(void) { m.t0 = SDL_GetTicks64(); g_eax = 0; g_esp += 8; }
void sub_004130A0(void) { m.t0 = 0; ds_track_clear(); g_eax = 0; g_esp += 8; }

/* BOOL complete(obj), stdcall: EC_COMPLETE came, i.e. the last frame was shown. */
void sub_00413110(void) {
    g_eax = m.done;
    g_esp += 8;
}

/* double position(obj), seconds in ST(0) */
void sub_00413150(void) { rt_fpush(elapsed()); g_esp += 4; }

/* int frame(obj, wait): the texture holding the frame due now, when a new one came; else with wait > 0 the
 * previous one, else -1 (the caller sleeps and asks again). */
void sub_00413170(void) {
    int fresh = 0;
    while (m.fmt && m.bits[0] && !m.done) {
        if (!m.have && !(m.have = decode_video())) { m.done = 1; break; }
        if (frame_time() > elapsed()) break;
        AVFrame *f = m.frame;
        m.sws = sws_getCachedContext(m.sws, f->width, f->height, f->format, 640, 480, AV_PIX_FMT_BGRA,
                                     SWS_BILINEAR, NULL, NULL, NULL);
        uint8_t *dst[4] = {GPTR(m.bits[m.next])};
        int pitch[4] = {640 * 4};
        if (m.sws) sws_scale(m.sws, (const uint8_t *const *)f->data, f->linesize, 0, f->height, dst, pitch);
        m.have = 0;
        fresh = 1;
    }
    if (fresh) {
        g_eax = m.next;
        m.next ^= 1;
    } else
        g_eax = (int32_t)ARG(1) > 0 ? (uint32_t)(m.next ^ 1) : 0xFFFFFFFFu;
    g_esp += 4;
}

/* HRESULT destroy(obj), stdcall */
void sub_00413B10(void) {
    if (ARG(0) == m.guest) {
        gfree(m.guest);
        close_movie();
    }
    g_eax = 0;
    g_esp += 8;
}

recomp_func_t movie_lookup(uint32_t va) {
    switch (va) {
    case 0x00413AC0u: return sub_00413AC0;
    case 0x00413880u: return sub_00413880;
    case 0x004131C0u: return sub_004131C0;
    case 0x00413130u: return sub_00413130;
    case 0x00413090u: return sub_00413090;
    case 0x004130A0u: return sub_004130A0;
    case 0x00413110u: return sub_00413110;
    case 0x00413150u: return sub_00413150;
    case 0x00413170u: return sub_00413170;
    case 0x00413B10u: return sub_00413B10;
    }
    return NULL;
}
