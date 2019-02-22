/**
 * @file ClipDecode.h
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the definition and usage for ClipDecode API:
 * These functions build ontop of clip_read_packet() to get a packet from the
 * original video file, decode it and return an AVFrame
 */

#ifndef _CLIP_DECODE_
#define _CLIP_DECODE_

#include "Clip.h"

/**
 * Read clip packet and decode it.
 * Will automatically skip by packets if they are before clip->seek_pts
 * This occurs because we need an I-frame before non I-frame packets to decode.
 * @param  clip         Clip to read
 * @param  frame        output of decoded packet between clip bounds (precise seeking!)
 * @param  frame_type   type of frame (AVMEDIA_TYPE_VIDEO/AVMEDIA_TYPE_AUDIO)
 * @return       >= 0 on success, < 0 when reached EOF, end of clip boundary or error.
 */
int clip_read_frame(Clip *clip, AVFrame *frame, enum AVMediaType *frame_type);

/*************** EXAMPLE FUNCTIONS ***************/
/**
 * Test example showing how to read frames from clips
 * @param clip Clip
 */
int example_clip_read_frames(Clip *clip);

/*************** INTERNAL FUNCTIONS **************/

/**
 * Detects if frame is before seek
 * @param  clip  Clip
 * @param  frame decoded frame
 * @param  type  AVMEDIA_TYPE_VIDEO/AVMEDIA_TYPE_AUDIO
 * @return       true if frame is before seek position, false otherwise
 */
bool frame_before_seek(Clip *clip, AVFrame *frame, enum AVMediaType type);

/**
 * Handle receive frame return, deciding to send another packet
 * @param  clip  Clip to read packets
 * @param  ret   return value from calling avcodec_receive_frame()
 * @return       >= 0 on success
 */
int handle_receive_frame(Clip *clip, AVFrame *frame, int ret, enum AVMediaType *type);

/**
 * Sends a single clip packet and get the decoded frame
 * @param  clip  Clip to read packet
 * @param  frame decoded frame output
 * @return       >= 0 on success
 */
int clip_send_packet_get_frame(Clip *clip, AVFrame *frame, enum AVMediaType *type);

/**
 * Send clip packet to decoder
 * @param  clip Clip to read packets
 * @return      >= 0 on success
 */
int clip_send_packet(Clip *clip);

#endif