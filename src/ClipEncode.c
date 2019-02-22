/**
 * @file ClipEncode.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the source for ClipEncode API:
 * These functions build ontop of ClipDecode API to get a raw AVFrame
 * from a clip which we use to encode
 */

#include "ClipEncode.h"

/**
 * Read packet from clip, decode it into AVFrame, and re-encode it for output
 * This function builds ontop of clip_read_frame() in ClipDecode.c
 * @param  oc           OutputContext already allocated with codecs
 * @param  clip         Clip to encode
 * @param  pkt          output encoded packet
 * @return      >= 0 on success, < 0 when reach EOF, end of clip boundary or error.
 */
int clip_encode_frame(OutputContext *oc, Clip *clip, AVPacket *pkt) {
    if(oc->video.done_flush && oc->audio.done_flush) {
        return AVERROR_EOF;
    } else {
        if(oc->last_encoder_frame_type == AVMEDIA_TYPE_VIDEO || oc->video.flushing) {
            return clip_receive_enc_packet(oc, &(oc->video), clip, pkt);
        } else if(oc->last_encoder_frame_type == AVMEDIA_TYPE_AUDIO || oc->audio.flushing) {
            return clip_receive_enc_packet(oc, &(oc->audio), clip, pkt);
        } else {
            return clip_send_frame_to_encoder(oc, clip, pkt);
        }
    }
}

/*************** EXAMPLE FUNCTIONS ***************/
/**
 * Encode all frames in a clip
 * @param  oc   OutputContext already allocated
 * @param  clip CLip to encode
 */
void example_clip_encode_frames(OutputContext *oc, Clip *clip) {
    AVPacket *pkt = av_packet_alloc();
    while(clip_encode_frame(oc, clip, pkt) >= 0) {
        printf("ClipEncode: ");
        if(pkt->stream_index == oc->video.stream->index) {
            printf("Video packet! pts: %ld\n", pkt->pts);
        } else if(pkt->stream_index == oc->audio.stream->index) {
            printf("Audio packet! pts: %ld\n", pkt->pts);
        }
    }
    av_packet_free(&pkt);
}

/*************** INTERNAL FUNCTIONS ***************/
/**
 * Receive an encoded packet given an output stream and handle the return value
 * @param  oc   OutputContext
 * @param  os   OutputStream within OutputContext (video or audio)
 * @param  clip Clip being read
 * @param  pkt  output encoded packet
 * @return      >= 0 on success
 */
int clip_receive_enc_packet(OutputContext *oc, OutputStream *os, Clip *clip, AVPacket *pkt) {
    int ret = avcodec_receive_packet(os->codec_ctx, pkt);
    if(ret == 0) {
        // successfully received packet from encoder
        // rescale packet timestamp values from codec to stream timebase
        av_packet_rescale_ts(pkt, os->codec_ctx->time_base, os->stream->time_base);
        pkt->stream_index = os->stream->index;  // set stream index to OutputStream index
        return ret;
    } else if(ret == AVERROR(EAGAIN)) {
        // output is not available in the current state - user must try to send input
        return clip_send_frame_to_encoder(oc, clip, pkt);
    } else if(ret == AVERROR_EOF) {
        // the encoder has been fully flushed, and there will be no more output frames
        os->flushing = false;
        os->done_flush = true;
        return clip_encode_frame(oc, clip, pkt);
    } else {
        // legitimate encoding errors
        fprintf(stderr, "Legitmate encoding error when handling receive frame[%s]\n",
                            av_err2str(ret));
        return ret;
    }
}

/**
 * Send a frame to encoder. This function builds ontop of clip_read_frame().
 * @param  oc   OutputContext already allocated
 * @param  clip Clip to encode
 * @param  pkt  packet output
 * @return      >= 0 on success
 */
int clip_send_frame_to_encoder(OutputContext *oc, Clip *clip, AVPacket *pkt) {
    enum AVMediaType type;
    // read decoded frame from clip
    int ret = clip_read_frame(clip, oc->buffer_frame, &type);
    // no more frames or error
    if(ret < 0) {
        oc->last_encoder_frame_type = AVMEDIA_TYPE_NB;

        // flush the video stream
        ret = avcodec_send_frame(oc->video.codec_ctx, NULL);
        if(ret < 0) {
            fprintf(stderr, "Failed to flush the video stream (%s)\n", av_err2str(ret));
            return ret;
        }
        oc->video.flushing = true;

        // flush the audio stream
        ret = avcodec_send_frame(oc->audio.codec_ctx, NULL);
        if(ret < 0) {
            fprintf(stderr, "Failed to flush the audio stream (%s)\n", av_err2str(ret));
            return ret;
        }
        oc->audio.flushing = true;

        // try to receive a flushed packet out
        return clip_encode_frame(oc, clip, pkt);
    }
    // oc->buffer_frame->key_frame = 1;
    oc->buffer_frame->pict_type = AV_PICTURE_TYPE_NONE;
    // supply a raw video or audio frame to the encoder
    if(type == AVMEDIA_TYPE_VIDEO) {
        ret = avcodec_send_frame(oc->video.codec_ctx, oc->buffer_frame);
        return handle_send_frame(oc, clip, type, ret, pkt);
    } else if(type == AVMEDIA_TYPE_AUDIO) {
        ret = avcodec_send_frame(oc->audio.codec_ctx, oc->buffer_frame);
        return handle_send_frame(oc, clip, type, ret, pkt);
    } else {
        // this should never happen
        fprintf(stderr, "AVFrame type is invalid (must be AVMEDIA_TYPE_VIDEO or AVMEDIA_TYPE_AUDIO)\n");
        return -1;
    }
}

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
int handle_send_frame(OutputContext *oc, Clip *clip, enum AVMediaType type, int ret, AVPacket *pkt) {
    if(ret == 0) {
        // successfully sent frame to encoder
        oc->last_encoder_frame_type = type;
        return clip_encode_frame(oc, clip, pkt);    // receive the packet from encoder
    } else if(ret == AVERROR(EAGAIN)) {
        // input is not accepted in the current state - user must read output
        return clip_encode_frame(oc, clip, pkt);
    } else {
        // legitimate encoding error
        fprintf(stderr, "Legitmate encoding error when handling send frame[%s]\n",
                            av_err2str(ret));
        return ret;
    }
}

