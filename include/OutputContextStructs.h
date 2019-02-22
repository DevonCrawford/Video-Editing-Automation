/**
 * @file OutputContextStructs.h
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the definition of OutputContext data structures
 */

#ifndef _OUTPUT_CONTEXT_STRUCTS_
#define _OUTPUT_CONTEXT_STRUCTS_

#include <stdio.h>
#include <stdbool.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>

typedef struct VideoOutParams {
    /*
        https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gaadca229ad2c20e060a14fec08a5cc7ce
        set to AV_CODEC_ID_NONE if you want the codec to automatically detect this
     */
    enum AVCodecID codec_id;
    // where c is AVCodecContext *c;
    /*
        c->pix_fmt
     */
    enum AVPixelFormat pix_fmt;
    /*
        c->width, c->height
     */
    int width, height;

    /*
        still need to work this out.. looks like I can control the bitrate
        here, but it causes issues like glitches at the start of clips.
        Can be set to -1 to allow default codec settings to control bitrate
     */
    int64_t bit_rate;

    /*
        used to get time_base of 1/fps.
     */
    int fps;
} VideoOutParams;

typedef struct AudioOutParams {
    /*
        https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gaadca229ad2c20e060a14fec08a5cc7ce
        set to AV_CODEC_ID_NONE if you want the codec to automatically detect this
     */
    enum AVCodecID codec_id;
    /*
        Audio sample format
        - encoding: set by user
        - decoding: set by libavcodec.sample format
        examples:
        AV_SAMPLE_FMT_FLT (float)
        AV_SAMPLE_FMT_S32 (signed 32 bits)
     */
    enum AVSampleFormat sample_fmt;
    /*
        the average bitrate (in bits per second)
        encoding: Set by user; unused for constant quantizer encoding.
        examples:
        1536000 (1.5MB/s)
     */
    int64_t bit_rate;
    /*
        samples per second for audio (the equivalent of fps in video)
        examples:
        48000   (48kHz)
        44100   (44.1kHz)
     */
    int sample_rate;
    /*
        https://www.ffmpeg.org/doxygen/trunk/group__channel__mask__c.html
        examples:
        AV_CH_LAYOUT_STEREO
        AV_CH_LAYOUT_MONO
     */
    uint64_t channel_layout;
} AudioOutParams;

typedef struct OutputParameters {
    VideoOutParams video;
    AudioOutParams audio;
    char *filename;
} OutputParameters;




typedef struct OutputStream {
    AVCodec *codec;
    AVCodecContext *codec_ctx;
    AVStream *stream;
    bool flushing, done_flush;
} OutputStream;

typedef struct OutputContext {
    AVFormatContext *fmt_ctx;
    OutputStream video, audio;
    AVFrame *buffer_frame;
    enum AVMediaType last_encoder_frame_type;
} OutputContext;

#endif