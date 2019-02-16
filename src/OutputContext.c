#include "OutputContext.h"

/**
 * Open format and file for video output
 * @param  fmt_ctx  out - AVFormatContext will be allocated automatically by file extension
 * @param  streams  input - incoming packets stream_index field must be set to the index of the
 *                  corresponding stream provided here. You should get this info from demuxing.
 *                  Read av_interleaved_write_frame() docs for more info on why we need this.
 * @param  filename input - name of output file to be created
 * @return          >= 0 on success
 */
int open_video_output(AVFormatContext *fmt_ctx, AVStream **streams, char *filename) {
    AVDictionary *opt = NULL;
    avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, filename);
    if (!fmt_ctx) {
        printf("Could not deduce output format from file extension: using MP4.\n");
        avformat_alloc_output_context2(&fmt_ctx, NULL, "mp4", filename);
    }
    if (!fmt_ctx) {
        fprintf(stderr, "Failed to allocate output context for file[%s]\n", filename);
        return -1;
    }
    fmt_ctx->streams = streams;

    int ret;
    /* open the output file, if needed */
    if(!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&(fmt_ctx->pb), filename, AVIO_FLAG_WRITE);
        if(ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", filename,
                    av_err2str(ret));
            return ret;
        }
    }

    ret = avformat_write_header(fmt_ctx, &opt);
    if(ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str(ret));
    }
    return ret;
}

/**
 * Close video output file
 * @param  fmt_ctx AVFormatContext containing open file information
 * @return         >= 0 on success
 */
int close_video_output(AVFormatContext *fmt_ctx) {
    int ret = av_write_trailer(fmt_ctx);
    if(ret < 0) {
        fprintf(stderr, "Failed to write trailer [%s]\n", fmt_ctx->url);
        return ret;
    }
    if(!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        /* Close the output file */
        ret = avio_closep(&(fmt_ctx->pb));
        if(ret < 0) {
            fprintf(stderr, "Failed to close output file [%s]\n", fmt_ctx->url);
        }
    }
    /* free the stream */
    avformat_free_context(fmt_ctx);
    return ret;
}