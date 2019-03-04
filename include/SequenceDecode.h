/**
 * @file SequenceDecode.h
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the definition and usage for SequenceDecode API:
 * These functions build ontop of ClipDecode API to read all decoded frames
 * from a list of clips in a sequence
 */

#ifndef _SEQUENCE_DECODE_
#define _SEQUENCE_DECODE_

#include "Sequence.h"
#include "ClipDecode.h"

/**
 * Read decoded frames from our editing sequence
 * @param  seq              Sequence with clips to be read
 * @param  frame            decoded output frame
 * @param  frame_type       type of output frame
 * @param  close_clips_flag if true, close clips after read
 * @return                  >= 0 on success (returned a frame)
 *                          < 0 when reached end of sequence or error
 */
int sequence_read_frame(Sequence *seq, AVFrame *frame, enum AVMediaType *frame_type, bool close_clips_flag);

/**
 * Clear fields on AVFrame from decoding
 * @param f AVFrame to initialize
 */
void clear_frame_decoding_garbage(AVFrame *f);

/*************** EXAMPLE FUNCTIONS ***************/
/**
 * Test example showing how to read frames from sequence
 * @param seq Sequence to read
 */
 int example_sequence_read_frames(Sequence *seq, bool close_clips_flag);

#endif