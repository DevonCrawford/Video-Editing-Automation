/**
 * @file test-clip.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File testing the Clip API
 */

#include "Timebase.h"
#include "Clip.h"

int cut_range(AVFormatContext *fmt_ctx, int64_t startFrame, int64_t endFrame);
int cut(AVFormatContext *fmt_ctx, int64_t startFrame, int64_t duration);

int main(int argc, char **argv) {
    VideoContext vid_ctx;
    open_video_context(&vid_ctx, argv[1]);
    /* dump input information to stderr */
    av_dump_format(vid_ctx.fmt_ctx, 0, argv[1], 0);

    AVStream *video_stream = vid_ctx.fmt_ctx->streams[vid_ctx.video_stream_idx];
    AVStream *audio_stream = vid_ctx.fmt_ctx->streams[vid_ctx.audio_stream_idx];
    printf("video_stream->duration: %ld\n", video_stream->duration);
    printf("video_stream->nb_frames: %ld\n", video_stream->nb_frames);

    // printf("video_dec_ctx->width: %d\n", vid_ctx.video_codec_ctx->width);
    // printf("video_dec_ctx->height: %d\n", vid_ctx.video_codec_ctx->height);

    printf("num: %d, den: %d\n", audio_stream->time_base.num, audio_stream->time_base.den);
    printf("start_time: %ld\n", audio_stream->nb_frames);
    int64_t video_pts = get_video_frame_pts(&vid_ctx, 177);
    int64_t audio_pts = get_audio_frame_pts(&vid_ctx, 177);
    printf("video_pts: %ld\n", video_pts);
    printf("audio_pts: %ld\n", audio_pts);

    printf("video_time_base_num: %d, video_time_base_den: %d\n", video_stream->time_base.num, video_stream->time_base.den);


    // Clip clip;
    // init_clip(&clip, argv[1]);
    // open_clip(&clip);
    // set_clip_bounds(&clip, (uint64_t)atoi(argv[2]), (uint64_t)atoi(argv[3]));
    // printf("end_frame_idx: %ld\n", get_clip_end_frame_idx(&clip));
    // example_clip_read_packets(&clip);
    // close_clip(&clip);
    // free_clip(&clip);

    // free video context
    close_video_context(&vid_ctx);
    return 0;
}