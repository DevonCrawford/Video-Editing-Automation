#ifndef _OUTPUT_CONTEXT_API_
#define _OUTPUT_CONTEXT_API_

#include <stdio.h>
#include <libavformat/avformat.h>

/**
 * Open format and file for video output
 * @param  fmt_ctx  out - AVFormatContext will be allocated automatically by file extension
 * @param  streams  input - incoming packets stream_index field must be set to the index of the
 *                  corresponding stream provided here. You should get this info from demuxing.
 *                  Read av_interleaved_write_frame() docs for more info on why we need this.
 * @param  filename input - name of output file to be created
 * @return          >= 0 on success
 */
 int open_video_output(AVFormatContext *fmt_ctx, AVStream **streams, char *filename);
 
/**
 * Close video output file
 * @param  fmt_ctx AVFormatContext containing open file information
 * @return         >= 0 on success
 */
int close_video_output(AVFormatContext *fmt_ctx);

#endif