/**
 * @file ClipDecode.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the source for ClipDecode API:
 * These functions build ontop of clip_read_packet() to get a packet from the
 * original video file, decode it and return an AVFrame
 */

#include "ClipDecode.h"

/**
 * Read clip packet and decode it.
 * Will automatically skip by packets if they are before clip->seek_pts
 * This occurs because we need an I-frame before non I-frame packets to decode.
 * @param  clip         Clip to read
 * @param  frame        output of decoded packet between clip bounds (precise seeking!)
 * @param  frame_type   type of frame (AVMEDIA_TYPE_VIDEO/AVMEDIA_TYPE_AUDIO)
 * @return       >= 0 on success, < 0 when reached EOF, end of clip boundary or error.
 */
int clip_read_frame(Clip *clip, AVFrame *frame, enum AVMediaType *frame_type) {
    VideoContext *vid_ctx = clip->vid_ctx;
    int ret, handle_ret = 0;
    do {
        // try to receive frame from decoder (from clip_send_packet())
        if(vid_ctx->last_decoder_packet_stream == DEC_STREAM_VIDEO) {
            *frame_type = AVMEDIA_TYPE_VIDEO;
            ret = avcodec_receive_frame(vid_ctx->video_codec_ctx, frame);
            if(ret == 0) {
                // success, frame was returned from decoder
                if(frame->pts < clip->vid_ctx->seek_pts) {
                    printf("skip video frame[%ld] before seek[%ld]\n", frame->pts, clip->vid_ctx->seek_pts);
                    handle_ret = 1;
                } else {
                    ++(clip->frame_index);
                    handle_ret = 0;
                }
            } else if(ret == AVERROR(EAGAIN)) {
                // output is not available in this state - user must try to send new input
                ret = clip_send_packet(clip);   // if no more packets or error
                if(ret < 0) {
                    clip->frame_index = 0;
                    return ret;
                }
                handle_ret = 1;
            } else {
                // legitimate decoding error
                fprintf(stderr, "Error decoding frame (%s)\n", av_err2str(ret));
                clip->frame_index = 0;
                return -1;
            }
        } else if(vid_ctx->last_decoder_packet_stream == DEC_STREAM_AUDIO) {
            *frame_type = AVMEDIA_TYPE_AUDIO;
            ret = avcodec_receive_frame(vid_ctx->audio_codec_ctx, frame);
            if(ret == 0) {
                // success, frame was returned from decoder
                int64_t seek_pts = cov_video_to_audio_pts(clip->vid_ctx, clip->vid_ctx->seek_pts);
                if(frame->pts < seek_pts) {
                    printf("skip audio frame[%ld] before seek[%ld]\n", frame->pts, seek_pts);
                    handle_ret = 1;
                } else {
                    handle_ret = 0;
                }
            } else if(ret == AVERROR(EAGAIN)) {
                // output is not available in this state - user must try to send new input
                ret = clip_send_packet(clip);   // if no more packets or error
                if(ret < 0) {
                    clip->frame_index = 0;
                    return ret;
                }
                handle_ret = 1;
            } else {
                // legitimate decoding error
                fprintf(stderr, "Error decoding frame (%s)\n", av_err2str(ret));
                clip->frame_index = 0;
                return -1;
            }
        } else {
            ret = clip_send_packet(clip);
            // if no more packets or error
            if(ret < 0) {
                clip->frame_index = 0;
                return ret;
            }
            handle_ret = 1;
        }
    } while(handle_ret == 1);
    return handle_ret;
}

/*************** EXAMPLE FUNCTIONS ***************/
/**
 * Test example showing how to read frames from clips
 * @param clip Clip
 */
int example_clip_read_frames(Clip *clip) {
    enum AVMediaType type;
    AVFrame *frame = av_frame_alloc();
    if(!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        return AVERROR(ENOMEM);
    }
    while(clip_read_frame(clip, frame, &type) >= 0) {
        printf("clip: %s | ", clip->vid_ctx->url);
        if(type == AVMEDIA_TYPE_VIDEO) {
            printf("Video frame! pts: %ld, frame: %ld\n", frame->pts,
                    cov_video_pts(clip->vid_ctx, frame->pts));
        } else if(type == AVMEDIA_TYPE_AUDIO) {
            printf("Audio frame! pts: %ld\n", frame->pts);
        }
    }
    av_frame_free(&frame);
    return 0;
}

/*************** INTERNAL FUNCTIONS **************/

/**
 * Detects if frame is before seek
 * @param  clip  Clip
 * @param  frame decoded frame
 * @param  type  AVMEDIA_TYPE_VIDEO/AVMEDIA_TYPE_AUDIO
 * @return       true if frame is before seek position, false otherwise
 */
bool frame_before_seek(Clip *clip, AVFrame *frame, enum AVMediaType type) {
    int64_t seek_pts = clip->vid_ctx->seek_pts;
    if(type == AVMEDIA_TYPE_AUDIO) {
        seek_pts = cov_video_to_audio_pts(clip->vid_ctx, seek_pts);
    }
    return frame->pts < seek_pts;
}

/**
 * Send clip packet to decoder
 * @param  clip Clip to read packets
 * @return      >= 0 on success
 */
int clip_send_packet(Clip *clip) {
    VideoContext *vid_ctx = clip->vid_ctx;
    AVPacket pkt;
    int ret = clip_read_packet(clip, &pkt);
    // no more packets, or error
    if(ret < 0 || !pkt.size) {
        vid_ctx->last_decoder_packet_stream = DEC_STREAM_NONE;
        return ret;
    }
    // Send raw packet to decoder
    if(pkt.stream_index == vid_ctx->video_stream_idx) {
        ret = avcodec_send_packet(vid_ctx->video_codec_ctx, &pkt);
        if(ret < 0) {
            fprintf(stderr, "Failed to send video packet to decoder (%s)\n", av_err2str(ret));
        } else {
            vid_ctx->last_decoder_packet_stream = DEC_STREAM_VIDEO;
        }
    }
    else if(pkt.stream_index == vid_ctx->audio_stream_idx) {
        ret = avcodec_send_packet(vid_ctx->audio_codec_ctx, &pkt);
        if(ret < 0) {
            fprintf(stderr, "Failed to send audio packet[%ld] to decoder (%s)\n",
                                pkt.pts, av_err2str(ret));
        } else {
            vid_ctx->last_decoder_packet_stream = DEC_STREAM_AUDIO;
        }
    } else {
        printf("packet is not video or audio stream. (we should never get down here, this should be handled internally by clip_read_packet())\n");
        ret = -1;
    }
    av_packet_unref(&pkt);
    return ret;
}