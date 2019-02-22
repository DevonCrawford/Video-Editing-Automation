/**
 * @file OutputContext.h
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the definition and usage for OutputContext API:
 * These functions build ontop of SequenceEncode API to write encoded
 * frames to an output file. 
 */

#ifndef _OUTPUT_CONTEXT_API_
#define _OUTPUT_CONTEXT_API_

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include "OutputContextStructs.h"
#include "SequenceEncode.h"

#include <libavutil/opt.h>

/**
 * Initialize output stream
 * @param os OutputStream
 */
void init_output_stream(OutputStream *os);

/**
 * Initialize OutputContext structure
 * @param out_ctx       OutputContext
 */
void init_video_output(OutputContext *oc);

/**
 * Add a stream and allocate codecs for an OutputStream
 * @param  oc       OutputContext
 * @param  os       OutputStream within OutputContext
 * @param  codec_id id of codec to allocate
 * @return          >= 0 on success
 */
int add_stream(OutputContext *oc, OutputStream *os, enum AVCodecID codec_id);

/**
 * Set video codec and muxer parameters from VideoOutParams
 * @param  oc OutputContext
 * @param  op VideoOutParams
 * @return    >= 0 on success
 */
int set_video_codec_params(OutputContext *oc, VideoOutParams *op);

/**
 * Set audio codec and muxer parameters from AudioOutParams
 * @param  oc OutputContext
 * @param  op AudioOutParams
 * @return    >= 0 on success
 */
int set_audio_codec_params(OutputContext *oc, AudioOutParams *op);

/**
 * copy params to muxer
 * @param  oc OutputContext
 * @param  os OutputStream (video or audio) within OutputContext
 * @param  c  AVCodecContext associated with video or audio stream.
 *              parameters from this will be copied to muxer
 * @return    >= 0 on success
 */
int set_muxer_params(OutputContext *oc, OutputStream *os, AVCodecContext *c);

/**
 * Open format and file for video output
 * @param  oc           OutputContext
 * @param  op           OutputParameters for video and audio codecs/muxers (filename as well)
 * @param  seq          Sequence to derive time_base for codec context
 * @return              >= 0 on success
 */
int open_video_output(OutputContext *oc, OutputParameters *op, Sequence *seq);

/**
 * Open output file, write sequence frames and close the output file!
 * (this is an end to end solution)
 * @param  seq      Sequence containing clips to write to file
 * @param  op       OutputParameters for video and audio codec/muxers (and filename)
 * @return          >= 0 on success
 */
int write_sequence(Sequence *seq, OutputParameters *op);

/**
 * Write entire sequence to an output file
 * @param  oc  OutputContext
 * @param  seq Sequence containing clips
 * @return     >= 0 on success
 */
int write_sequence_frames(OutputContext *oc, Sequence *seq);

 /**
  * Set OutputParameters given Video and Audio OutputParameters
  * @param op        OutputParameters to set filename, video and audio params
  * @param filename  name of output video file
  * @param vp        Video params
  * @param ap        Audio params
  * @return          >= 0 on success
  */
 int set_output_params(OutputParameters *op, char *filename, VideoOutParams vp, AudioOutParams ap);

 /**
  * Copy relevant codec context params (from decoder) into VideoOutParams struct (for encoding)
  * This can copy clip decoder settings to the encoder context
  * @param  op VideoOutParams
  * @param  c  AVCodecContext containing data (ideally from decoder)
  */
 void set_video_out_params(VideoOutParams *op, AVCodecContext *c);

 /**
  * Copy relevant codec context params (from decoder) into AudioOutParams struct (for encoding)
  * @param  op AudioOutParams
  * @param  c  AVCodecContext containing data (ideally from decoder)
  */
 void set_audio_out_params(AudioOutParams *op, AVCodecContext *c);

 /**
  * Free OutputParameters internal data
  * @param op OutputParameters to be freed
  */
 void free_output_params(OutputParameters *op);

 /**
  * Close the codec context within output stream
  * @param os OutputStream with open AVCodecContext
  */
 void close_output_stream(OutputStream *os);

 /**
  * Close video output file
  * @param out_ctx
  * @param trailer_flag when true, trailer will attempt to be written
  * @return         >= 0 on success
  */
 int close_video_output(OutputContext *out_ctx, bool trailer_flag);

#endif