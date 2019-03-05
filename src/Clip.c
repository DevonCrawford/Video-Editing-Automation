/**
 * @file Clip.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the source code for Clip API:
 * Clip stores a reference to a video file and its data within an editing sequence
 */

#include "Clip.h"

/**
 * Allocate a new clip pointing to the same VideoContext as Clip param.
 * Returned Clip has its own Clip fields, but references the same VideoContext as src.
 * This is useful for using the same video file with multiple clips
 * @param  src new clip will point to same VideoContext as this clip
 * @return     >= 0 on success
 */
Clip *copy_clip_vc(Clip *src) {
    if(src == NULL || src->vid_ctx == NULL) {
        fprintf(stderr, "copy_clip_vc() error: Invalid params\n");
        return NULL;
    }
    Clip *copy = alloc_clip_internal();
    if(copy == NULL) {
        fprintf(stderr, "copy_clip_vc() error: Failed to allocate new clip\n");
        return NULL;
    }
    copy->vid_ctx = src->vid_ctx;
    ++(copy->vid_ctx->clip_count);
    return copy;
}

/**
 * Allocate a new Clip without a VideoContext
 * @return NULL on fail, not NULL on success
 */
Clip *alloc_clip_internal() {
    Clip *clip = malloc(sizeof(struct Clip));
    if(clip == NULL || init_clip_internal(clip) < 0) {
        return NULL;
    }
    return clip;
}
/**
 * Allocate clip on heap, initialize default values and open the clip.
 * It is important to open the clip when first created so we can set default
 * values such as clip->orig_end_pts by reading file contents (length of video)
 * @param  url filename
 * @return     NULL on error, not NULL on success
 */
Clip *alloc_clip(char *url) {
    Clip *clip = malloc(sizeof(struct Clip));
    if(clip == NULL ) {
        return NULL;
    } else if(init_clip(clip, url) < 0 || open_clip(clip) < 0) {
        free_clip(&clip);
    }
    return clip;
}

/**
 * Initialize clip without video context
 * @param  clip Clip to initialize
 * @return      >= 0 on success
 */
int init_clip_internal(Clip *clip) {
    if(clip == NULL) {
        fprintf(stderr, "init_clip_internal() error: Invalid params\n");
        return -1;
    }
    clip->vid_ctx = NULL;
    clip->orig_start_pts = 0;
    clip->orig_end_pts = -1;
    clip->start_pts = -1;
    clip->end_pts = -1;
    clip->done_reading_video = false;
    clip->done_reading_audio = false;
    clip->frame_index = 0;
    return 0;
}

/**
 * Initialize Clip with new VideoContext
 * @param  clip Clip to be initialized
 * @param  url  filename
 * @return      >= 0 on success
 */
int init_clip(Clip *clip, char *url) {
    int ret = init_clip_internal(clip);
    if(ret < 0) {
        return ret;
    }
    clip->vid_ctx = (VideoContext *)malloc(sizeof(struct VideoContext));
    if(clip->vid_ctx == NULL) {
        fprintf(stderr, "init_clip() error: Failed to allocate vid_ctx\n");
        return -1;
    }
    init_video_context(clip->vid_ctx);
    clip->vid_ctx->clip_count = 1;
    clip->vid_ctx->url = malloc(strlen(url) + 1);
    if(clip->vid_ctx->url == NULL) {
        fprintf(stderr, "init_clip() error: Failed to allocate video_context filename[%s]\n", url);
        return -1;
    }
    strcpy(clip->vid_ctx->url, url);
    return 0;
}

/**
 * Open a clip (VideoContext) to read data from the original file
 * @param  clip Clip with videoContext to be opened
 * @return      >= 0 on success
 */
int open_clip(Clip *clip) {
    if(clip == NULL) {
        fprintf(stderr, "open_clip() error: NULL param\n");
        return -1;
    }
    if(!(clip->vid_ctx->open)) {
        int ret;
        if((ret = open_video_context(clip->vid_ctx, clip->vid_ctx->url)) < 0) {
            fprintf(stderr, "open_clip() error: Failed to open VideoContext for clip[%s]\n", clip->vid_ctx->url);
            // free_video_context(&(clip->vid_ctx));
            return ret;
        }
        AVStream *video_stream = get_video_stream(clip->vid_ctx);
        if(clip->orig_end_pts == -1) {
            clip->orig_end_pts = video_stream->duration;
        }
        init_internal_vars(clip);
        ret = seek_clip_pts(clip, 0);
        if(ret < 0) {
            return ret;
        }
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
    close_video_context(clip->vid_ctx);
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
    Initialize Clip object with start and endpoints
    @param clip: Clip to initialize
    @param vid_ctx: Video context assumed to already be initialized
    @param orig_start_pts: start pts in original video file
    @param orig_end_pts: end pts in original video file
    Return >= 0 on success
*/
int set_clip_bounds_pts(Clip *clip, int64_t orig_start_pts, int64_t orig_end_pts) {
    int ret;
    if((ret = set_clip_start(clip, orig_start_pts)) < 0) {
        return ret;
    }
    if((ret = set_clip_end(clip, orig_end_pts)) < 0) {
        return ret;
    }
    return 0;
}

/**
    Based on fps of original footage..
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
    if(pts < 0) {
        return -1;
    }
    int ret = seek_video_pts(clip->vid_ctx, pts);
    if(ret >= 0) {
        clip->orig_start_pts = pts;
        clip->vid_ctx->seek_pts = pts;
        clip->vid_ctx->curr_pts = pts;
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
    clip->orig_end_pts = pts;
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
    return seek_clip_pts(clip, pts);
}

/**
    Seek to a clip frame, relative to the clip.
    @param Clip: clip to seek
    @param pts: relative pts to the clip (where zero represents clip->orig_start_pts)
*/
int seek_clip_pts(Clip *clip, int64_t pts) {
    // get absolute index (so ffmpeg can seek on video file)
    int64_t abs_pts = pts + clip->orig_start_pts;
    if(pts < 0 || abs_pts > clip->orig_end_pts) {
        int64_t endBound = get_clip_end_frame_idx(clip);
        fprintf(stderr, "seek_clip_pts() error: seek pts[%ld] outside of clip bounds (0 - %ld)\n", pts, endBound);
        return -1;
    }
    int ret;
    if((ret = seek_video_pts(clip->vid_ctx, abs_pts)) < 0) {
        fprintf(stderr, "seek_clip_pts() error: Failed to seek to pts[%ld] on clip[%s]\n", abs_pts, clip->vid_ctx->url);
        return ret;
    }
    clip->vid_ctx->seek_pts = abs_pts;
    if((ret = cov_video_pts(clip->vid_ctx, abs_pts)) < 0) {
        fprintf(stderr, "seek_clip_pts error: Failed to convert pts to frame index\n");
        return ret;
    }
    clip->vid_ctx->curr_pts = clip->vid_ctx->seek_pts;
    return 0;
}

/**
 * Gets absolute pts for clip (the pts of original video file)
 * @param  clip         Clip
 * @param  relative_pts pts relative to clip (where zero represents clip->orig_start_pts)
 * @return              absolute pts of clip. (relative to original video file)
 */
int64_t get_abs_clip_pts(Clip *clip, int64_t relative_pts) {
    return relative_pts + clip->orig_start_pts;
}

/**
 * Convert absolute pts (from VideoContext), into a pts relative to the Clip bounds
 * @param  clip    Clips
 * @param  abs_pts absolute pts (relative to original video)
 * @return         abs_pts - clip->orig_start_pts
 *                 >= 0 if abs_pts is within start boundary
 */
int64_t cov_clip_pts_relative(Clip *clip, int64_t abs_pts) {
    return abs_pts - clip->orig_start_pts;
}

/**
 * Convert raw video packet timestamp into clip relative pts
 * @param  clip    Clip
 * @param  pkt_pts raw packet pts from original video file
 * @return         pts relative to clip video
 */
int64_t clip_ts_video(Clip *clip, int64_t pkt_ts){
    return pkt_ts - clip->orig_start_pts;
}

/**
 * Convert raw video packet timestamp into clip relative audio pts
 * @param  clip    Clip
 * @param  pkt_pts raw packet pts from original video file (in video timebase)
 * @return         pts relative to clip audio timebase
 */
int64_t clip_ts_audio(Clip *clip, int64_t pkt_ts) {
    return pkt_ts - cov_video_to_audio_pts(clip->vid_ctx, clip->orig_start_pts);
}

/**
    Gets index of last frame in clip
    @param clip: Clip to get end frame
    Return >= 0 on success
*/
int64_t get_clip_end_frame_idx(Clip* clip) {
    return cov_video_pts(clip->vid_ctx, clip->orig_end_pts - clip->orig_start_pts);
}

/**
 * Determine if VideoContext is out of clip bounds.
 * If this is the case, then VideoContext was used by another clip and needs
 * to be reset on the current clip with seek_clip_pts(clip, 0);
 * @param  clip Clip
 * @return      true if VideoContext is out of clip bounds
 */
bool is_vc_out_bounds(Clip *clip) {
    return clip->vid_ctx->seek_pts < clip->orig_start_pts ||
            clip->vid_ctx->seek_pts >= clip->orig_end_pts;
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

    int64_t video_end_pts = clip->orig_end_pts;
    int64_t audio_end_pts = cov_video_to_audio_pts(vid_ctx, video_end_pts);
    if(tmpPkt.stream_index == vid_ctx->video_stream_idx) {
        // If packet is past, or equal to the end_frame_pts (outside of clip)
        // This performs exclusive end_pts
        if(tmpPkt.pts >= video_end_pts) {
            clip->done_reading_video = true;

            // Recursion to read the rest of audio frames
            if(!clip->done_reading_audio) {
                av_packet_unref(&tmpPkt);
                return clip_read_packet(clip, pkt);
            }
        } else {
            *pkt = tmpPkt;
            clip->vid_ctx->curr_pts = pkt->pts;
        }
    } else if(tmpPkt.stream_index == vid_ctx->audio_stream_idx) {
        if(tmpPkt.pts >= audio_end_pts) {
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
 * Compare two clips based on pts
 * @param  first  First clip to compare
 * @param  second Second clip to compare
 * @return        > 0 if first->pts > second->pts.  (first after second)
 *                < 0 if first->pts < second->pts.  (second after first)
 *                  0 if first->pts = second->pts.  (first and second at same pts)
 */
int64_t compare_clips(Clip *first, Clip *second) {
    if(first == NULL || second == NULL) {
        fprintf(stderr, "compare_clips() error: params cannot be NULL\n");
        return -2;
    }
    return first->start_pts - second->start_pts;
}

/**
 * Compare clips first by date & time and then second by orig_start_pts
 * @param  f First clip to compare
 * @param  s Second clip to compare
 * @return   0 if date, time and orig_start_pts are equal.
 *           > 0 if first is greater than second
 *           < 0 if first is less than second
 */
int64_t compare_clips_sequential(Clip *f, Clip *s) {
    if(f == NULL || s == NULL) {
        fprintf(stderr, "compare_clips_sequential() error: params cannot be NULL\n");
        return -2;
    }
    double diff = difftime(f->vid_ctx->file_stats.st_mtime, s->vid_ctx->file_stats.st_mtime);
    if(diff < 0.01 && diff > -0.01) {
        // if equal date & time
        // compare start pts
        return f->orig_start_pts - s->orig_start_pts;
    } else if(diff > 0) {
        return 1;
    } else if(diff < 0) {
        return -1;
    }
    fprintf(stderr, "compare_clips_sequential() error: We should never get down here..\n");
    return -2;
}

/**
 * Get time_base of clip video stream
 * @param  clip Clip
 * @return      time_base of clip video stream
 */
AVRational get_clip_video_time_base(Clip *clip) {
    if(!clip->vid_ctx->open) {
        fprintf(stderr, "Failed to get video time_base: clip[%s] is not open\n", clip->vid_ctx->url);
        return (AVRational){-1, -1};
    }
    return get_video_time_base(clip->vid_ctx);
}

/**
 * Get time_base of clip audio stream
 * @param  clip Clip
 * @return      time_base of clip audio stream
 */
AVRational get_clip_audio_time_base(Clip *clip) {
    if(!clip->vid_ctx->open) {
        fprintf(stderr, "Failed to get audio time_base: clip[%s] is not open\n", clip->vid_ctx->url);
        return (AVRational){-1, -1};
    }
    return get_audio_time_base(clip->vid_ctx);
}

/**
 * Get video stream from clip
 * @param  clip Clip containing VideoContext
 * @return      video AVStream on success, NULL on failure
 */
AVStream *get_clip_video_stream(Clip *clip) {
    if(!clip->vid_ctx->open) {
        fprintf(stderr, "Failed to get video stream: clip[%s] is not open\n", clip->vid_ctx->url);
        return NULL;
    }
    return get_video_stream(clip->vid_ctx);
}

/**
 * Get audio stream from clip
 * @param  clip Clip containing VideoContext
 * @return      audio AVStream on success, NULL on failure
 */
AVStream *get_clip_audio_stream(Clip *clip) {
    if(!clip->vid_ctx->open) {
        fprintf(stderr, "Failed to get audio stream: clip[%s] is not open\n", clip->vid_ctx->url);
        return NULL;
    }
    return get_audio_stream(clip->vid_ctx);
}

/**
 * Get codec parameters of clip video stream
 * @param  clip Clip to get parameters
 * @return      not NULL on success
 */
AVCodecParameters *get_clip_video_params(Clip *clip) {
    AVCodecParameters *par = avcodec_parameters_alloc();
    int ret = avcodec_parameters_copy(par, get_clip_video_stream(clip)->codecpar);
    if(ret < 0) {
        avcodec_parameters_free(&par);
        par = NULL;
        fprintf(stderr, "Failed to get clip[%s] video params\n", clip->vid_ctx->url);
    } else {
        if(par->extradata) {
            free(par->extradata);
            par->extradata = NULL;
            par->extradata_size = 0;
        }
    }
    return par;
}

/**
 * Get codec parameters of clip audio stream
 * @param  clip Clip to get parameters
 * @return      not NULL on success
 */
AVCodecParameters *get_clip_audio_params(Clip *clip) {
    AVCodecParameters *par = avcodec_parameters_alloc();
    int ret = avcodec_parameters_copy(par, get_clip_audio_stream(clip)->codecpar);
    if(ret < 0) {
        avcodec_parameters_free(&par);
        par = NULL;
        fprintf(stderr, "Failed to get clip[%s] audio params\n", clip->vid_ctx->url);
    } else{
        if(par->extradata) {
            free(par->extradata);
            par->extradata = NULL;
            par->extradata_size = 0;
        }
    }
    return par;
}

/**
 * Cut a clip, splitting it in two.
 * This function handles clip positions (orig_start_pts and orig_end_pts) of clips but not
 * sequence positions (start_pts or end_pts) of clips. Sequence positions are handled in cut_clip() in Sequence.c
 * @param  oc   Original Clip to be cut
 * @param  pts  pts relative to clip and clip timebase (zero represents orig_start_pts)
 *              , where the highest value is the orig_end_pts
 * @param  sc   newly created clip with set bounds (second half of split)
 * @return
 */
int cut_clip_internal(Clip *oc, int64_t pts, Clip **sc) {
    *sc = NULL;
    if(oc == NULL || oc->vid_ctx == NULL) {
        fprintf(stderr, "cut_clip_internal() error: Invalid params\n");
        return -1;
    }
    AVStream *vid_stream = get_clip_video_stream(oc);
    int64_t frame_duration = vid_stream->duration / vid_stream->nb_frames;
    if((pts < frame_duration) || (pts >= (oc->orig_end_pts - oc->orig_start_pts))) {
        printf("cut_clip_internal(): pts out of range/cannot cut less than one frame.. ");
        printf("pts: %ld, frame_duration: %ld\n", pts, frame_duration);
        return -1;
    }
    // set orig_end_pts of original clip
    int64_t sc_orig_end_pts = oc->orig_end_pts;
    int ret = set_clip_end(oc, oc->orig_start_pts + pts);
    if(ret < 0) {
        return ret;
    }
    *sc = copy_clip_vc(oc);
    if(*sc == NULL) {
        fprintf(stderr, "cut_clip_internal() error: failed to allocate new clip\n");
        return -1;
    }
    ret = set_clip_bounds_pts(*sc, oc->orig_start_pts + pts, sc_orig_end_pts);
    if(ret < 0) {
        free_clip(sc);
        fprintf(stderr, "cut_clip_internal() error: Failed to set bounds on new clip\n");
        return ret;
    }
    return 0;
}

/**
    Frees data within clip structure and the Clip allocation itself
*/
void free_clip(Clip **clip) {
    Clip *c = *clip;
    if(c->vid_ctx) {
        if(c->vid_ctx->clip_count <= 1) {
            free_video_context(&(c->vid_ctx));
        } else {
            --(c->vid_ctx->clip_count);
        }
    }
    free(*clip);
    *clip = NULL;
}

/*************** LINKED LIST FUNCTIONS ***************/
/**
 * Get data about a clip in a string
 * @param  toBePrinted Clip containing data
 * @return             data in string
 */
char *list_print_clip(void *toBePrinted) {
    char *str;
    if(toBePrinted == NULL) {
        str = malloc(sizeof(char));
        str[0] = 0;
    } else {
        Clip *clip = (Clip *) toBePrinted;
        char buf[1024];
        sprintf(buf, "start_pts: %ld\norig_start_pts: %ld\norig_end_pts: %ld\n",
                        clip->start_pts, clip->orig_start_pts, clip->orig_end_pts);
        char *str = malloc(sizeof(char) * (strlen(buf) + 1));
        strcpy(str, buf);
    }
    return str;
}

/**
 * Free clip memory and all its data. Assumes clip was allocated on heap.
 * @param toBeDeleted Clip structure allocated on heap
 */
void list_delete_clip(void *toBeDeleted) {
    if(toBeDeleted == NULL) {
        return;
    }
    Clip *clip = (Clip *) toBeDeleted;
    free_clip(&clip);
}

/**
 * Compare two clips within a linked list by start_pts (timestamp in sequence)
 * (uses compare_clips() internally)
 * @param  first  first clip to compare
 * @param  second second clip to compare
 * @return        > 0 if first->pts > second->pts.  (first after second)
 *                < 0 if first->pts < second->pts.  (second after first)
 *                  0 if first->pts = second->pts.  (first and second at same pts)
 */
int list_compare_clips(const void *first, const void *second) {
    if(first == NULL || second == NULL) {
        fprintf(stderr, "ERROR: compare clips param cannot be NULL\n");
        return -2;
    }
    Clip *clip1 = (Clip *) first;
    Clip *clip2 = (Clip *) second;
    int64_t diff = compare_clips(clip1, clip2);
    if(diff > 0) {
        return 1;
    } else if(diff < 0) {
        return -1;
    } else {
        return 0;
    }
}

/**
 * Compare two clips within a linked list by date & time first, then orig_start_pts second.
 * This is useful for ordering clips by time they were shot.
 * Use insertSorted with this compare function to insert clips into a sequence in sequential order
 * @param  first  first clip to compare
 * @param  second second clip to compare
 * @return        > 0 if first->pts > second->pts.  (first after second)
 *                < 0 if first->pts < second->pts.  (second after first)
 *                  0 if first->pts = second->pts.  (first and second at same pts)
 */
int list_compare_clips_sequential(const void *first, const void *second) {
    if(first == NULL || second == NULL) {
        fprintf(stderr, "list_compare_clips_sequential() error: params cannot be NULL\n");
        return -2;
    }
    Clip *f = (Clip *) first;
    Clip *s = (Clip *) second;
    int64_t diff = compare_clips_sequential(f, s);
    if(diff > 0) {
        return 1;
    } else if(diff < 0) {
        return -1;
    } else {
        return 0;
    }
}


/*************** EXAMPLE FUNCTIONS ***************/
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

