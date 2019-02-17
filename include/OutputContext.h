#ifndef _OUTPUT_CONTEXT_API_
#define _OUTPUT_CONTEXT_API_

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include "Sequence.h"

typedef struct OutputContext {
    AVFormatContext *fmt_ctx;
    AVStream *video_stream, *audio_stream;
} OutputContext;

/**
 * Initialize OutputContext structure
 * @param out_ctx OutputContext
 */
void init_video_output(OutputContext *out_ctx);

/**
 * Open format and file for video output
 * @param  out_ctx      OutputContext
 * @param  filename     name of output video file
 * @param  exampleClip  Clip to pull codec parameters for muxer (output codec)
 * @return          >= 0 on success
 */
int open_video_output(OutputContext *out_ctx, char *filename, Clip *exampleClip);

/**
 * Open output file, write sequence packets and close the output file!
 * (this is an end to end solution)
 * @param  seq          Sequence containing clips to write to file
 * @param  filename     name of output video (extension determines container type)
 * @param  exampleClip  Clip to pull codec parameters for muxer (output codec)
 * @return          >= 0 on success
 */
int write_sequence(Sequence *seq, char *filename, Clip *exampleClip);

 /**
  * Write all sequence packets to an output file!
  * @param  fmt_ctx AVFormatContext already assumed to be allocated with open_video_output()
  * @param  seq     Sequence containing clips
  * @return         >= 0 on success
  */
 int write_sequence_packets(OutputContext *out_ctx, Sequence *seq);

 /**
  * Close video output file
  * @param out_ctx
  * @param trailer_flag when true, trailer will attempt to be written
  * @return         >= 0 on success
  */
 int close_video_output(OutputContext *out_ctx, bool trailer_flag);

#endif