/**
 * @file VideoContext.h
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the definition and usage for VideoContext API:
 * The primary data structure for raw media files, videos just as they
 * are on disk.
 * Higher level structures such as Clip build ontop of this.
 */

#ifndef _VIDEOCONTEXT_API_
#define _VIDEOCONTEXT_API_

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <stdbool.h>
#include <sys/stat.h>

enum PacketStreamType { DEC_STREAM_NONE = -1, DEC_STREAM_VIDEO, DEC_STREAM_AUDIO };

typedef struct VideoContext {
    /*
        Open file information
     */
    AVFormatContext *fmt_ctx;           // fmt_ctx->streams[stream_index] contains all streams
    /*
        Codecs used for decoding.
        Encoding is handled by OutputContext, since the entire sequence
        needs to be encoded with the same codec and codec parameters
     */
    AVCodec *video_codec, *audio_codec;
    /*
        CodecContexts used for demuxing
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

    /*
        video context open or closed state
    */
    bool open;

    /*
        File stats such as creation time.
        "sb.st_mtime" is what we want.
        This will show the last modified date of the file (creation time).
     */
    struct stat file_stats;

    /*
        filename
    */
    char *url;

    /*
        Timebases fetched when the file is first opened.
        After the file is open, get timebase from here to avoid opening the file again
     */
    AVRational video_time_base, audio_time_base;

    /*
        frames per second of original video
     */
    double fps;

    /*
        pts of seek, absolute to original video pts.
        time_base is the same as VideoContext video_stream time_base
        RANGE:
        >= orig_start_pts and < orig_end_pts of Clip.
     */
    int64_t seek_pts;

    /*
    By default this is start_frame_pts, but will change when reading packets.
    (We track this by video packets seen and seek usage)
     */
    int64_t curr_pts;

    /*
        number of clips associated with this VideoContext.
        We must only free a VideoContext when the last clip using it is freed
     */
    int clip_count;
} VideoContext;

# define VIDEO_CONTEXT_STREAM_TYPES_LEN 2

AVStream *get_video_stream(VideoContext *vid_ctx);
AVStream *get_audio_stream(VideoContext *vid_ctx);
AVRational get_video_time_base(VideoContext *vid_ctx);
AVRational get_audio_time_base(VideoContext *vid_ctx);
/*
    out @param vid_ctx - allocated context containing format, streams and codecs
    in @param filename - name of video file
    Return >= 0 if OK, < 0 on fail
*/
int open_video_context(VideoContext *vid_ctx, char *filename);

/* Return >=0 if OK, < 0 on fail */
int open_format_context(VideoContext *vid_ctx, char *filename);

/**
    Finds a stream and its associated codec
    Call this function after open_format_context() - for each AVMediaType
    Returns >= 0 if OK, < 0 on fail
*/
int open_codec_context(VideoContext *vid_ctx, enum AVMediaType type);


void init_video_context(VideoContext *vid_ctx);

/** free codecs and ffmpeg struct data inside VideoContext **/
void close_video_context(VideoContext *vid_ctx);

/**
 * Free all videoContext data (including url)
 * @param vid_ctx VideoContext
 */
void free_video_context(VideoContext **vc);

/**
 * Check if AVRational is valid
 * @param  r AVRational to check
 * @return   true if invalid
 */
bool valid_rational(AVRational r);

/**
 * Print most data within an AVCodecContext
 * @param  c AVCodecContext with data
 * @return   char * allocated on heap, it is the callers responsibility to free
 */
char *print_codec_context(AVCodecContext *c);

#endif