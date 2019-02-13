#include "videoContext.h"

int cut_range(AVFormatContext *fmt_ctx, int64_t startFrame, int64_t endFrame);
int cut(AVFormatContext *fmt_ctx, int64_t startFrame, int64_t duration);

int main(int argc, char **argv) {
    VideoContext vid_ctx;
    open_video(&vid_ctx, argv[1]);
    /* dump input information to stderr */
    av_dump_format(vid_ctx.fmt_ctx, 0, argv[1], 0);

    AVStream *video_stream = vid_ctx.fmt_ctx->streams[vid_ctx.video_stream_idx];
    AVStream *audio_stream = vid_ctx.fmt_ctx->streams[vid_ctx.audio_stream_idx];
    printf("video_stream->duration: %ld\n", video_stream->duration);
    printf("video_stream->nb_frames: %ld\n", video_stream->nb_frames);

    printf("video_dec_ctx->width: %d\n", vid_ctx.video_dec_ctx->width);
    printf("video_dec_ctx->height: %d\n", vid_ctx.video_dec_ctx->height);

    // free video context
    free_video_context(&vid_ctx);
    return 0;
}



// int cut_range(AVFormatContext *fmt_ctx, int64_t startFrame, int64_t endFrame) {
//     return cut(filename, startFrame, endFrame-startFrame);
// }
//
// int cut(AVFormatContext *fmt_ctx, int64_t startFrame, int64_t duration) {
//
// }