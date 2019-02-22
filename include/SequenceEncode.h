/**
 * @file SequenceEncode.h
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the definition and usage for SequenceEncode API:
 * These functions build ontop of SequenceDecode API to encode all frames
 * from a list of clips in a sequence
 */

#ifndef _SEQUENCE_ENCODE_
#define _SEQUENCE_ENCODE_

#include "SequenceDecode.h"
#include "OutputContextStructs.h"

/**
 * Read encodec packets from a sequence
 * This function builds ontop of sequence_read_frame() in SequenceDecode.c
 * @param  oc           OutputContext already allocated with codecs
 * @param  seq_ts       Sequence containing clips to encode
 * @param  pkt          output encoded packet
 * @return      >= 0 on success, < 0 when reach EOF, end of clip boundary or error.
 */
int sequence_encode_frame(OutputContext *oc, Sequence *seq, AVPacket *pkt);

/*************** EXAMPLE FUNCTIONS ***************/
/**
 * Test example showing how to encode frames from sequence
 * @param seq Sequence to encode
 */
 void example_sequence_encode_frames(OutputContext *oc, Sequence *seq);

/*************** INTERNAL FUNCTIONS ***************/
/**
 * Receive an encoded packet given an output stream, and handle the return value
 * @param  oc   OutputContext
 * @param  os   OutputStream within OutputContext (video or audio)
 * @param  seq  Sequence being encoded
 * @param  pkt  output encoded packet
 * @return      >= 0 on success
 */
int seq_receive_enc_packet(OutputContext *oc, OutputStream *os, Sequence *seq, AVPacket *pkt);

/**
 * Send a frame to encoder. This function builds ontop of seq_read_frame().
 * @param  oc   OutputContext already allocated
 * @param  seq  Sequence to encode
 * @param  pkt  packet output
 * @return      >= 0 on success
 */
int seq_send_frame_to_encoder(OutputContext *oc, Sequence *seq, AVPacket *pkt);

/**
 * Handle the return from avcodec_send_frame().
 * This function is to be used inside of seq_send_frame_to_encoder()
 * @param  oc   OutputContext already allocated
 * @param  seq  Sequence to encode
 * @param  type AVMediaType of frame that was sent
 * @param  ret  return from avcodec_send_frame()
 * @param  pkt  output packet
 * @return      >= 0 on success
 */
int seq_handle_send_frame(OutputContext *oc, Sequence *seq, enum AVMediaType type, int ret, AVPacket *pkt);

#endif