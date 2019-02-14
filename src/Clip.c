#include "Clip.h"

/**
    Initialize Clip object to full length of original video file
    Return >= 0 on success
*/
int init_clip(Clip *clip, char *url) {
    clip->url = NULL, clip->vid_ctx = NULL;
    clip->url = malloc(strlen(url) + 1);
    if(clip->url == NULL) {
        fprintf(stderr, "Failed to allocate clip url[%s]\n", url);
        return -1;
    }
    strcpy(clip->url, url);

    VideoContext *vid_ctx = (VideoContext *)malloc(sizeof(struct VideoContext));
    if(vid_ctx == NULL) {
        fprintf(stderr, "Failed to allocate vid_ctx\n");
        return -1;
    }
    init_video_context(vid_ctx);
    clip->vid_ctx = vid_ctx;
    clip->start_frame_pts = 0;
    clip->end_frame_pts = -1;
    clip->pts = 0;
    clip->current_frame_idx = 0;
    clip->open = false;
    return 0;
}

/*
    Open a clip (VideoContext) to read data from the original file
    Return >= 0 if OK, < 0 on fail
*/
int open_clip(Clip *clip) {
    if(!clip->open) {
        int ret;
        if((ret = open_video(clip->vid_ctx, clip->url)) < 0) {
            fprintf(stderr, "Failed to open VideoContext for clip[%s]\n", clip->url);
            return ret;
        }
        clip->open = true;
        clip->end_frame_pts = get_video_stream(clip->vid_ctx)->duration;
        init_internal_vars(clip);
    }
    return 0;
}

int open_clip_bounds(Clip *clip, int64_t start_idx, int64_t end_idx) {
    int ret;
    if((ret = open_clip(clip)) < 0) {
        return ret;
    }
    return set_clip_bounds(clip, start_idx, end_idx);
}

void close_clip(Clip *clip) {
    if(clip->open) {
        free_video_context(clip->vid_ctx);
        clip->open = false;
    }
}

/**
    Initialize Clip object with start and endpoints
    @param clip: Clip to initialize
    @param vid_ctx: Video context assumed to already be initialized
    @param start_idx: start frame number in original video file
    @param end_idx: end frame number in original video file
    Return >= 0 on success
*/
int set_clip_bounds(Clip *clip, int64_t start_idx, int64_t end_idx) {
    int ret;
    if((ret = set_clip_start_frame(clip, start_idx)) < 0) {
        return ret;
    }
    if((ret = set_clip_end_frame(clip, end_idx)) < 0) {
        return ret;
    }
    return 0;
}

/**
    Return >= 0 on success
*/
int set_clip_start_frame(Clip *clip, int64_t frameIndex) {
    int64_t pts = get_video_frame_pts(clip->vid_ctx, frameIndex);
    return set_clip_start(clip, pts);
}

/**
    Return >= 0 on success
*/
int set_clip_start(Clip *clip, int64_t pts) {
    if(pts < 0 || pts > clip->end_frame_pts) {
        return -1;
    }
    int ret = seek_video_pts(clip->vid_ctx, pts);
    if(ret >= 0) {
        clip->start_frame_pts = ret;
        clip->current_frame_idx = ret;
    }
    return ret;
}

/**
    Return >= 0 on success
*/
int set_clip_end_frame(Clip *clip, int64_t frameIndex) {
    int64_t pts = get_video_frame_pts(clip->vid_ctx, frameIndex);
    return set_clip_end(clip, pts);
}

/**
    Return >= 0 on success
*/
int set_clip_end(Clip *clip, int64_t pts) {
    if(pts < 0) {
        return -1;
    }
    clip->end_frame_pts = pts;
    return 0;
}

/**
    Seek to a clip frame, relative to the clip.
    @param Clip: clip to seek
    @param seekFrameIndex: frame index within the clip to seek
*/
int seek_clip(Clip *clip, int64_t seekFrameIndex) {
    int64_t pts = get_video_frame_pts(clip->vid_ctx, seekFrameIndex);
    if(pts < 0) {
        return -1;
    }
    pts += clip->start_frame_pts;
    if(pts > clip->end_frame_pts) {
        int64_t endBound = get_clip_end_frame_idx(clip);
        fprintf(stderr, "seek frame[%ld] out of clip bounds (0 - %ld)\n",
                                seekFrameIndex, endBound);
        return -1;
    }
    int ret;
    if((ret = seek_video_pts(clip->vid_ctx, pts)) < 0) {
        fprintf(stderr, "Failed to seek to frame[%ld] on clip[%s]\n", seekFrameIndex, clip->url);
        return ret;
    }
    if((ret = cov_video_pts(clip->vid_ctx, pts)) < 0) {
        fprintf(stderr, "Failed to convert pts to frame index\n");
        return ret;
    }
    clip->current_frame_idx = ret;
    return 0;
}

/**
    Gets index of last frame in clip
    @param clip: Clip to get end frame
    Return >= 0 on success
*/
int64_t get_clip_end_frame_idx(Clip* clip) {
    return cov_video_pts(clip->vid_ctx, clip->end_frame_pts - clip->start_frame_pts);
}

/**
 * Detects if we are done reading the current packet stream..
 * if true.. then the packet in parameter should be skipped over!
 * @param  clip clip
 * @param  pkt  AVPacket currently read
 * @return      true, if our clip has read the entire stream associated with this packet.
 *              false, otherwise
 */
bool done_curr_pkt_stream(Clip *clip, AVPacket *pkt) {
    return (clip->done_reading_audio && pkt->stream_index == clip->vid_ctx->audio_stream_idx)
        || (clip->done_reading_video && pkt->stream_index == clip->vid_ctx->video_stream_idx);
}

/**
    Read a single AVPacket from clip.
    Only returns packets if they are within clip boundaries (start_frame_pts - end_frame_pts)
    This is a wrapper on av_read_frame() from ffmpeg. It will increment the packet
    counter by 1 on each call. The idea is to call this function in a loop while return >= 0.
    When end of clip is reached the packet pointer will be reset to start of clip (seek 0)
    @param Clip to read packets
    @param pkt: output AVPacket when valid packet is read.. otherwise NULL
    Returns >= 0 on success. < 0 when reached EOF, end of clip boundary or error.
    End of clip only happens when both audio and video streams have reached their corresponding end_pts
    AVPacket is returned in param pkt.
    Return stream_idx when video or audio stream is complete.
*/
int clip_read_packet(Clip *clip, AVPacket *pkt) {
    AVPacket tmpPkt;
    VideoContext *vid_ctx = clip->vid_ctx;
    int ret, readPackets = 0;
    do {
        if(readPackets++ > 0) {
            av_packet_unref(&tmpPkt);
        }
        // If EOF (or error)
        if((ret = av_read_frame(vid_ctx->fmt_ctx, &tmpPkt)) < 0) {
            *pkt = tmpPkt;
            reset_packet_counter(clip);
            return ret;
        }
    } while(done_curr_pkt_stream(clip, &tmpPkt));   // Skip by packets from finished stream

    int64_t video_end_pts = clip->end_frame_pts;
    int64_t audio_end_pts = cov_video_to_audio_pts(vid_ctx, video_end_pts);
    if(tmpPkt.stream_index == vid_ctx->video_stream_idx) {
        // If packet is past the end_frame_pts (outside of clip)
        if(tmpPkt.pts > video_end_pts) {
            clip->done_reading_video = true;

            // Recursion to read the rest of audio frames
            if(!clip->done_reading_audio) {
                av_packet_unref(&tmpPkt);
                return clip_read_packet(clip, pkt);
            }
        } else {
            *pkt = tmpPkt;
            if((ret = cov_video_pts(vid_ctx, tmpPkt.pts)) < 0) {
                fprintf(stderr, "Failed to convert pts to frame index\n");
                return ret;
            }
            clip->current_frame_idx = ret;
        }
    } else if(tmpPkt.stream_index == vid_ctx->audio_stream_idx) {
        if(tmpPkt.pts > audio_end_pts) {
            clip->done_reading_audio = true;

            // Recursion to read the rest of video frames
            if(!clip->done_reading_video) {
                av_packet_unref(&tmpPkt);
                return clip_read_packet(clip, pkt);
            }
        } else {
            *pkt = tmpPkt;
        }
    } else {
        printf("Unknown packet type (not video or audio).. ignoring\n");
    }
    // If both audio and video streams have completed read cycle of the entire clip
    if(clip->done_reading_video && clip->done_reading_audio) {
        av_packet_unref(&tmpPkt);
        reset_packet_counter(clip);
        return -1;
    }
    return 0;
}

/**
 * Reset packet counter to beginning of clip and reset internal reading flags (to allow another read cycle)
 * @param  clip Clip
 * @return      >= 0 on success
 */
int reset_packet_counter(Clip *clip) {
    int ref;
    if((ref = seek_clip(clip, 0)) < 0) {    // Reset packet counter to start of clip
        return ref;
    }
    init_internal_vars(clip);
    return 0;
}

/**
 * Set internal vars used in reading stream data
 * @param clip Clip
 */
void init_internal_vars(Clip *clip) {
    clip->done_reading_video = (clip->vid_ctx->video_stream_idx == -1);
    clip->done_reading_audio = (clip->vid_ctx->audio_stream_idx == -1);
}

/**
    Frees data within clip structure (does not free Clip allocation itself)
*/
void free_clip(Clip *clip) {
    if(clip->open) {
        close_clip(clip);
    }
    free(clip->vid_ctx);
    free(clip->url);
    clip->vid_ctx = NULL;
    clip->url = NULL;
}

/**
 * Test example showing how to read packets from clips
 * @param clip Clip
 */
void example_clip_read_packets(Clip *clip) {
    AVPacket pkt;
    while(clip_read_packet(clip, &pkt) >= 0) {
        AVPacket orig_pkt = pkt;

        if(pkt.stream_index == clip->vid_ctx->video_stream_idx) {
            printf("Video packet! pts: %ld, frame: %ld\n", pkt.pts,
                    cov_video_pts(clip->vid_ctx, pkt.pts));
        } else if(pkt.stream_index == clip->vid_ctx->audio_stream_idx) {
            printf("Audio packet! pts: %ld\n", pkt.pts);
        }

        av_packet_unref(&orig_pkt);
    }
}

