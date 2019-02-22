/**
 * @file ClipEncode.h
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the definition and usage for ClipEncode API:
 * These functions build ontop of ClipDecode API to get a raw AVFrame
 * from a clip which we use to encode
 */

#ifndef _CLIP_ENCODE_
#define _CLIP_ENCODE_

#include "ClipDecode.h"
#include "OutputContext.h"

/**
 * Read packet from clip, decode it into AVFrame, and re-encode it for output
 * This function builds ontop of clip_read_frame() in ClipDecode.c
 * @param  oc           OutputContext already allocated with codecs
 * @param  clip         Clip to encode
 * @param  pkt          output encoded packet
 * @return      >= 0 on success, < 0 when reach EOF, end of clip boundary or error.
 */
int clip_encode_frame(OutputContext *oc, Clip *clip, AVPacket *pkt);

/*************** EXAMPLE FUNCTIONS ***************/
/**
 * Encode all frames in a clip
 * @param  oc   OutputContext already allocated
 * @param  clip CLip to encode
 */
void example_clip_encode_frames(OutputContext *oc, Clip *clip);

/*************** INTERNAL FUNCTIONS ***************/
/**
 * Receive an encoded packet given an output stream and handle the return value
 * @param  oc   OutputContext
 * @param  os   OutputStream within OutputContext (video or audio)
 * @param  clip Clip being read
 * @param  pkt  output encoded packet
 * @return      >= 0 on success
 */
int clip_receive_enc_packet(OutputContext *oc, OutputStream *os, Clip *clip, AVPacket *pkt);

/**
 * Send a frame to encoder. This function builds ontop of clip_read_frame().
 * @param  oc   OutputContext already allocated
 * @param  clip Clip to encode
 * @param  pkt  packet output
 * @return      >= 0 on success
 */
int clip_send_frame_to_encoder(OutputContext *oc, Clip *clip, AVPacket *pkt);

/**
 * Handle the return from avcodec_send_frame().
 * This function is to be used inside of clip_send_frame_to_encoder()
 * @param  oc   OutputContext already allocated
 * @param  clip Clip to encode
 * @param  type AVMediaType of frame that was sent
 * @param  ret  return from avcodec_send_frame()
 * @param  pkt  output packet
 * @return      >= 0 on success
 */
int handle_send_frame(OutputContext *oc, Clip *clip, enum AVMediaType type, int ret, AVPacket *pkt);

#endif