#ifndef _VIDEOCONTEXT_API_
#define _VIDEOCONTEXT_API_

#include <stdio.h>
#include <libavformat/avformat.h>

enum PacketStreamType { DEC_STREAM_NONE = -1, DEC_STREAM_VIDEO, DEC_STREAM_AUDIO };

typedef struct VideoContext {
    /*
        Open file information
     */
    AVFormatContext *fmt_ctx;           // fmt_ctx->streams[stream_index] contains all streams
    /*
        Codecs used for encoding/decoding
     */
    AVCodec *video_codec, *audio_codec;
    /*
        CodecContexts used for muxing/demuxing
     */
    AVCodecContext *video_codec_ctx, *audio_codec_ctx;
    /*
        index of stream in fmt_ctx->streams[stream_index]
     */
    int video_stream_idx, audio_stream_idx;
    /*
        internal use only. used in clip_read_frame()
     */
    enum PacketStreamType last_decoder_packet_stream;
} VideoContext;

# define VIDEO_CONTEXT_STREAM_TYPES_LEN 2

AVStream *get_video_stream(VideoContext *vid_ctx);
AVStream *get_audio_stream(VideoContext *vid_ctx);
AVRational get_video_time_base(VideoContext *vid_ctx);
AVRational get_audio_time_base(VideoContext *vid_ctx);
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