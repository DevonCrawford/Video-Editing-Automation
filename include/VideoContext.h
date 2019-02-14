#ifndef _VIDEOCONTEXT_API_
#define _VIDEOCONTEXT_API_

#include <stdio.h>
#include <libavformat/avformat.h>

typedef struct VideoContext {
    AVFormatContext *fmt_ctx;           // fmt_ctx->streams[stream_index] contains all streams
    AVCodec *video_dec, *audio_dec;
    AVCodecContext *video_dec_ctx, *audio_dec_ctx;
    int video_stream_idx, audio_stream_idx;
} VideoContext;

# define VIDEO_CONTEXT_STREAM_TYPES_LEN 2

AVStream *get_video_stream(VideoContext *vid_ctx);
AVStream *get_audio_stream(VideoContext *vid_ctx);
/*
    out @param vid_ctx - allocated context containing format, streams and codecs
    in @param filename - name of video file
*/
int open_video(VideoContext *vid_ctx, char *filename);

/* Return >=0 if OK, < 0 on fail */
int open_format_context(VideoContext *vid_ctx, char *filename);

/**
    Finds a stream and its associated codec
    Call this function after open_format_context() - for each AVMediaType
    Returns >= 0 if OK, < 0 on fail
*/
int open_codec_context(VideoContext *vid_ctx, enum AVMediaType type);


void init_video_context(VideoContext *vid_ctx);

void free_video_context(VideoContext *vid_ctx);

#endif