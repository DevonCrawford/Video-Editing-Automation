/**
 * @file Sequence.c
 * @author Devon Crawford
 * @date February 14, 2019
 * @brief File containing the definitions and usage of the Sequence API:
 * A sequence is a list of clips in the editing timeline
 */

#include "Sequence.h"

/**
 * Initialize new sequence and list of clips
 * @param  sequence     Sequence is assumed to already be allocated memory
 * @param  fps          frames per second for the video stream. Video time base = 1/(fps*1000)
 *                      this fps is required and will base all functions that ask for
 *                      frameIndex.. ex: 30fps and frameIndex of 30 would be 1 second in.
 * @param  sample_rate  sample rate of the audio stream. Audio time base = 1/sample_rate
 * @return              >= 0 on success
 */
int init_sequence(Sequence *seq, double fps, int sample_rate) {
    return init_sequence_cmp(seq, fps, sample_rate, &list_compare_clips);
}

/**
 * Initialize a new sequence, list of clips along with a custom compare function (for insertSorted)
 * @param  seq         Sequence to initialize
 * @param  fps         frames per second
 * @param  sample_rate sample_rate of the audio stream
 * @param  compareFunc custom compare function used in sorting and searching
 * @return             >= 0 on success
 */
int init_sequence_cmp(Sequence *seq, double fps, int sample_rate, int (*compareFunc)(const void* first,const void* second)) {
    if(seq == NULL || compareFunc == NULL) {
        fprintf(stderr, "init_sequence_cmp() error: params cannot be NULL\n");
        return -1;
    }
    seq->clips = initializeList(&list_print_clip, &list_delete_clip, compareFunc);
    seq->clips_iter = createIterator(seq->clips);
    seq->video_time_base = (AVRational){1, fps * SEQ_VIDEO_FRAME_DURATION};
    seq->audio_time_base = (AVRational){1, sample_rate};
    seq->fps = fps;
    seq->video_frame_duration = SEQ_VIDEO_FRAME_DURATION;
    return 0;
}

/**
 * Allocate a clip within a sequence and use a reference to the same videoContext
 * for clips with the same url (filename)
 * @param  seq Sequence where clip will be added (used to find videoContext)
 * @param  url filename of clip to create
 * @return     >= 0 on success
 */
Clip *seq_alloc_clip(Sequence *seq, char *url) {
    Clip *clip = malloc(sizeof(struct Clip));
    if(clip == NULL) {
        return NULL;
    }
    Clip *seq_clip = find_clip(seq, url);
    if(seq_clip == NULL) {
        init_clip(clip, url);
    } else {
        init_clip_internal(clip);
        clip->vid_ctx = seq_clip->vid_ctx;
    }
    return clip;
}

/**
 * Find clip with the search url in a sequence
 * @param  seq Sequence containing clips to be searched
 * @param  url search key
 * @return     NULL on fail, not NULL on success
 */
Clip *find_clip(Sequence *seq, char *url) {
    if(seq == NULL || url == NULL) {
        fprintf(stderr, "find_clip() error: Invalid params\n");
    }
    Node *currNode = seq->clips.head;
    while(currNode != NULL) {
        Clip *clip = (Clip *) currNode->data;
        if(strcmp(clip->vid_ctx->url, url) == 0) {
            return clip;
        }
        currNode = currNode->next;
    }
    return NULL;
}
/**
 * Get duration of sequence in frames (defined by fps)
 * @param  seq Sequence
 * @return     >= 0 on success
 */
int64_t get_sequence_duration(Sequence *seq) {
    int64_t pts_dur = get_sequence_duration_pts(seq);
    if(pts_dur < 0) {
        return pts_dur;
    }
    return seq_pts_to_frame_index(seq, pts_dur);
}

/**
 * Get duration of sequence in pts (sequence timebase)
 * @param  seq Sequence
 * @return     >= 0 on success
 */
int64_t get_sequence_duration_pts(Sequence *seq) {
    if(seq == NULL) {
        return -1;
    }
    if(seq->clips.tail == NULL) {
        return 0;
    }
    return ((Clip *)(seq->clips.tail->data))->end_pts;
}

/**
 * Insert Clip in sequence in sorted clip->pts order
 * @param  sequence Sequence containing a list of clips
 * @param  clip     Clip to be added into the sequence list of clips
 * @param  start_frame_index
 * @return          >= 0 on success
 */
void sequence_add_clip(Sequence *seq, Clip *clip, int start_frame_index) {
    if(seq == NULL || clip == NULL) {
        fprintf(stderr, "sequence_add_clip error: parameters cannot be NULL");
        return;
    }
    int64_t pts = get_video_frame_pts(clip->vid_ctx, start_frame_index);
    sequence_add_clip_pts(seq, clip, pts);
}

/**
 * Insert Clip in sequence in sorted clip->pts order
 * @param  sequence Sequence containing a list of clips
 * @param  clip     Clip to be added into the sequence list of clips
 * @param  start_pts
 * @return          >= 0 on success
 */
void sequence_add_clip_pts(Sequence *seq, Clip *clip, int64_t start_pts) {
    printf("sequence add clip [%s], start_pts: %ld\n", clip->vid_ctx->url, start_pts);
    if(seq == NULL || clip == NULL) {
        fprintf(stderr, "sequence_add_clip error: parameters cannot be NULL");
        return;
    }
    move_clip_pts(seq, clip, start_pts);
    insertSorted(&(seq->clips), clip);
    if(seq->clips.length == 1) {
        seq->clips_iter.current = seq->clips.head;
    }
}

/**
 * Add clip to the end of sequence
 * @param seq  Sequence to insert clip
 * @param clip Clip to be inserted into end of sequence
 */
void sequence_append_clip(Sequence *seq, Clip *clip) {
    if(seq == NULL || clip == NULL) {
        fprintf(stderr, "sequence_add_clip error: parameters cannot be NULL");
        return;
    }
    void *data = getFromBack(seq->clips);
    if(data == NULL) {
        sequence_add_clip_pts(seq, clip, 0);
    } else {
        Clip *last = (Clip *) data;
        int64_t start_pts = last->end_pts;
        sequence_add_clip_pts(seq, clip, start_pts);
    }
}

/**
 * Insert clip sorted by:
 * 1. Date & time of file
 * 2. clip->orig_start_pts
 * This function will generate the sequence pts for the clip (clip->start_pts and clip->end_pts)
 * @param  seq  Sequence
 * @param  clip Clip to insert
 * @return      >= 0 on success
 */
int sequence_insert_clip_sorted(Sequence *seq, Clip *clip) {
    if(seq == NULL || clip == NULL) {
        fprintf(stderr, "sequence_insert_clip_sorted() error: parameters cannot be NULL\n");
        return -1;
    }
    Node *node = insertSortedGetNode(&(seq->clips), clip);
    if(node == NULL) {
        fprintf(stderr, "sequence_insert_clip_sorted() error: could not insert clip in sorted order\n");
        return -1;
    }
    seq->clips_iter.current = seq->clips.head;
    if(node->previous == NULL) {
        move_clip_pts(seq, clip, 0);
    } else {
        move_clip_pts(seq, clip, ((Clip *) (node->previous->data))->end_pts);
    }
    return shift_clips_after(seq, node);
}

/**
 * Shift clips sequence pts to after the current node (insert clip function)
 * @param  seq          Sequence
 * @param  curr_node    current clip which should shift all following nodes
 * @return      >= 0 on success
 */
int shift_clips_after(Sequence *seq, Node *curr_node) {
    if(seq == NULL || curr_node == NULL) {
        fprintf(stderr, "shift_clips_after() error: parameters cannot be NULL\n");
        return -1;
    }
    Clip *curr = (Clip *) (curr_node->data);
    int64_t shift = curr->end_pts - curr->start_pts;
    Node *node = curr_node->next;
    while(node != NULL) {
        Clip *next = (Clip *) (node->data);
        next->start_pts += shift;
        next->end_pts += shift;
        node = node->next;
    }
    return 0;
}

/**
 * Delete a clip from a sequence and move all following clips forward
 * @param  seq  Sequence
 * @param  clip Clip to delete
 * @return      >= 0 on success
 */
int sequence_ripple_delete_clip(Sequence *seq, Clip *clip) {
    if(seq == NULL || clip == NULL) {
        fprintf(stderr, "sequence_delete_clip() error: parameters cannot be NULL");
        return -1;
    }
    Node *curr = getNodeFromData(&(seq->clips), clip);
    if(curr == NULL) {
        fprintf(stderr, "sequence_delete_clip() error: clip data does not exist in sequence\n");
        return -1;
    }
    Node *next = curr->next;
    void *data = deleteDataFromList(&(seq->clips), clip);
    if(data == NULL) {
        fprintf(stderr, "sequence_delete_clip() error: Failed to delete clip from sequence\n");
        return -1;
    }
    if(next == NULL) {
        list_delete_clip(data);
        return 0;
    }
    Clip *c = (Clip *) (next->data);
    int64_t shift = c->start_pts - ((Clip *) data)->start_pts;
    list_delete_clip(data);
    do {
        c = (Clip *) (next->data);
        c->start_pts -= shift;
        c->end_pts -= shift;
        next = next->next;
    } while(next != NULL);
    return 0;
}

/**
 * Convert sequence frame index to pts (presentation time stamp)
 * @param  seq         Sequence
 * @param  frame_index index of frame to convert
 * @return             pts representation of frame in sequence
 */
int64_t seq_frame_index_to_pts(Sequence *seq, int frame_index) {
    if(seq == NULL || frame_index < 0) {
        fprintf(stderr, "seq_frame_index_to_pts() error: Invalid parameters\n");
        return -1;
    }
    return seq->video_frame_duration * frame_index;
}

/**
 * Convert pts to frame index in sequence
 * @param  seq Sequence
 * @param  pts presentation time stamp to convert into frame index
 * @return     presentation time stamp representation of frame index
 */
int seq_pts_to_frame_index(Sequence *seq, int64_t pts) {
    if(seq == NULL || pts < 0) {
        fprintf(stderr, "seq_pts_to_frame_index() error: Invalid parameters\n");
        return -1;
    }
    return pts / seq->video_frame_duration;
}

/**
 * Cut a clip within a sequence, splitting the clip in two
 * @param  seq         Sequence
 * @param  frame_index index of frame in sequence (clip must lie at this point)
 * @return             >= 0 on success
 */
int cut_clip(Sequence *seq, int frame_index) {
    if(seq == NULL || frame_index < 0) {
        return -1;
    }
    Clip *clip = NULL, *split_clip = NULL;
    int64_t clip_pts = find_clip_at_index(seq, frame_index, &clip);
    if(clip == NULL) {
        fprintf(stderr, "cut_clip() error: Failed to find clip at index\n");
        return -1;
    }
    int ret = cut_clip_internal(clip, clip_pts, &split_clip);
    if(ret < 0 || ret == 1) {
        return ret;
    }
    split_clip->end_pts = clip->end_pts;
    int64_t frame_index_pts = seq_frame_index_to_pts(seq, frame_index);
    split_clip->start_pts = frame_index_pts;
    clip->end_pts = frame_index_pts;
    insertSorted(&(seq->clips), split_clip);
    return 0;
}

/**
 * Iterate all sequence clips to find the clip that contains this frame_index in sequence
 * @param  seq         Sequence
 * @param  frame_index index of frame in sequence
 * @param  found_clip  output clip if found. If not found this will be NULL
 * @return             >= 0 on success, -1 on fail.
 *                      The success number will be the pts relative to the clip,
 *                      and clip timebase (where zero represents clip->orig_start_pts)
 */
int64_t find_clip_at_index(Sequence *seq, int frame_index, Clip **found_clip) {
    Node *currNode = seq->clips.head;
    while(currNode != NULL) {
        Clip *clip = (Clip *) currNode->data;
        int64_t clip_pts;
        // If clip is found at this frame index (in sequence)
        if((clip_pts = seq_frame_within_clip(seq, clip, frame_index)) >= 0) {
            *found_clip = clip;
            return clip_pts;
        }
        currNode = currNode->next;
    }
    *found_clip = NULL;
    return -1;
}

/**
 * Determine if sequence frame lies within a clip (assuming clip is within sequence)
 * Example:
 * If this[xxx] is a clip where |---| is VideoContext: |---[XXXXX]-----|
 * then return of 0 would be the first X and return of 1 would be the second X and so on..
 * We can use the successful return of this function with seek_clip_pts() to seek within the clip!!
 * @param  sequence    Sequence containing clip
 * @param  clip        Clip within sequence
 * @param  frame_index index in sequence
 * @return             on success: pts relative to clip, and clip timebase (where zero represents clip->orig_start_pts)
 *                     on fail: < 0
 */
int64_t seq_frame_within_clip(Sequence *seq, Clip *clip, int frame_index) {
    int64_t seq_pts = seq_frame_index_to_pts(seq, frame_index);
    int64_t pts_diff = seq_pts - clip->start_pts; // find pts relative to the clip!
    // if sequence frame is within the clip
    if(pts_diff >= 0 && seq_pts < clip->end_pts) {
        // convert relative pts to clip time_base
        AVRational clip_tb = clip->vid_ctx->video_time_base;
        if(clip_tb.num < 0 || clip_tb.den < 0) {
            return -1;
        }
        return av_rescale_q(pts_diff, seq->video_time_base, clip_tb);
    } else {
        return -1;
    }
}

/**
 * Seek to an exact frame within the sequence (and all the clips within it)!
 * @param  seq         Sequence containing clips
 * @param  frame_index index in sequence, will be used to find clip and the clip frame
 * @return             >= 0 on success
 */
int sequence_seek(Sequence *seq, int frame_index) {
    Node *currNode = seq->clips.head;
    while(currNode != NULL) {
        Clip *clip = (Clip *) currNode->data;
        int64_t clip_pts;
        // If clip is found at this frame index (in sequence)
        if((clip_pts = seq_frame_within_clip(seq, clip, frame_index)) >= 0) {
            if(seq->clips_iter.current != NULL) {
                Clip *previous = (Clip *) seq->clips_iter.current->data;
                // If clips are different, close previous
                if(compare_clips(clip, previous) != 0) {
                    close_clip(previous);
                }
            }
            seq->clips_iter.current = currNode;
            // seek to the correct pts within the clip!
            return seek_clip_pts(clip, clip_pts);
        }
        currNode = currNode->next;
    }
    // If we got down here, then we did not find a clip at this frame index
    fprintf(stderr, "Failed to find a clip at sequence frame index[%d] :(\n", frame_index);
    return -1;
}

/**
 * Read our editing sequence!
 * This will iterate our clips wherever sequence_seek() left off
 * This function uses clip_read_packet() and av_read_frame() internally. This function
 * works the exact same.. reading one packet at a time and incrementing internally.
 * (call this function in a loop while >= 0 to get full edit)
 * @param  seq Sequence containing clips
 * @param  pkt output AVPacket
 * @param close_clips when true, each clip will be closed at the end of its read cycle (and reopened if read again)
 *          closing clips after usage will save memory (RAM) but take more clock cycles.
 *          it takes roughly 10ms to open a clip.
 *          maybe the most CPU efficient solution is opening the clips before usage of this function (and keeping them open for a while).
 *          However, there is the concern that opening many clips will use too much RAM
 *          (closing clips after usage would solve this issue)
 * @return     >= 0 on success (returns packet.stream_index), < 0 when reached end of sequence or error.
 */
int sequence_read_packet(Sequence *seq, AVPacket *pkt, bool close_clips_flag) {
    Node *currNode = seq->clips_iter.current;
    if(currNode == NULL) {
        printf("sequence_read_packet() currNode == NULL\n");
        return -1;
    }
    Clip *curr_clip = (Clip *) currNode->data;    // current clip
    AVPacket tmpPkt;
    int ret = clip_read_packet(curr_clip, &tmpPkt);
    // End of clip!
    if(ret < 0) {
        printf("End of clip[%s]\n", curr_clip->vid_ctx->url);
        if(close_clips_flag) {
            close_clip(curr_clip);
        }
        // move iterator to next element
        nextElement(&(seq->clips_iter));
        void *next = seq->clips_iter.current;       // get next clip Node
        if(next == NULL) {
            // We're done reading all clips! (reset to start)
            printf("We're done reading all clips! (reset to start)\n");
            sequence_seek(seq, 0);
            return -1;
        } else {
            // move onto next clip
            Clip *next_clip = (Clip *) ((Node *)next)->data;
            ret = open_clip(next_clip);
            if(ret < 0) {
                return ret;
            }
            return sequence_read_packet(seq, pkt, close_clips_flag);
        }
    } else {
        // valid packet down here
        *pkt = tmpPkt;
        return tmpPkt.stream_index;
    }
}

/**
 * Sets the start_pts of a clip in sequence
 * @param  seq               Sequence containing clip
 * @param  clip              Clip to set start_pts
 * @param  start_frame_index frame index in sequence to start the clip
 * @return                   >= 0 on success
 */
int move_clip(Sequence *seq, Clip *clip, int start_frame_index) {
    int64_t pts = get_video_frame_pts(clip->vid_ctx, start_frame_index);
    if(pts < 0) {
        return pts;
    }
    move_clip_pts(seq, clip, pts);
    return 0;
}

/**
 * Sets the start_pts of a clip in sequence
 * @param  seq               Sequence containing clip
 * @param  clip              Clip to set start_pts
 * @param  start_pts         pts in sequence to start the clip
 * @return                   >= 0 on success
 */
void move_clip_pts(Sequence *seq, Clip *clip, int64_t start_pts) {
    clip->start_pts = start_pts;
    // Automatically set the end_pts to the duration of the clip (which is set by set_clip_bounds())
    int64_t clip_dur = clip->orig_end_pts - clip->orig_start_pts;
    int64_t seq_dur = av_rescale_q(clip_dur, get_clip_video_time_base(clip),
                                             seq->video_time_base);
    clip->end_pts = start_pts + seq_dur;
}

/**
 * Get current clip from sequence (seek position for next read)
 * @param  seq Sequence containing clips
 * @return     Clip that is currently being read
 */
Clip *get_current_clip(Sequence *seq) {
    Node *curr = seq->clips_iter.current;
    if(curr == NULL) {
        return NULL;
    } else {
        return (Clip *) curr->data;
    }
}

/**
 * Convert a raw packet timestamp into a sequence timestamp
 * @param  seq          Sequence containing clip
 * @param  clip         Clip within sequence
 * @param  orig_pkt_ts  timestamp from AVPacket read directly from file (or clip_read_packet())
 * @return              timestamp representation of the video packet in the editing sequence!
 */
int64_t video_pkt_to_seq_ts(Sequence *seq, Clip *clip, int64_t orig_pkt_ts) {
    int64_t clip_ts = clip_ts_video(clip, orig_pkt_ts);
    AVRational clip_tb = get_clip_video_time_base(clip);
    if(clip_tb.num < 0 || clip_tb.den < 0) {
        fprintf(stderr, "video time_base is invalid for clip[%s]\n", clip->vid_ctx->url);
        return -1;
    }
    // rescale clip_ts to sequence time_base
    int64_t seq_ts = av_rescale_q(clip_ts, clip_tb, seq->video_time_base);
    // add clip starting point in sequence
    return clip->start_pts + seq_ts;
}

/**
 * Convert a raw packet timestamp into a sequence timestamp
 * @param  seq          Sequence containing clip
 * @param  clip         Clip within sequence
 * @param  orig_pkt_ts  timestamp from AVPacket read directly from file (or clip_read_packet())
 * @return              timestamp representation of the audio packet in the editing sequence!
 */
int64_t audio_pkt_to_seq_ts(Sequence *seq, Clip *clip, int64_t orig_pkt_ts) {
    int64_t clip_ts = clip_ts_audio(clip, orig_pkt_ts);
    AVRational clip_tb = get_clip_audio_time_base(clip);
    if(clip_tb.num < 0 || clip_tb.den < 0) {
        fprintf(stderr, "audio time_base is invalid for clip[%s]\n", clip->vid_ctx->url);
        return -1;
    }
    // rescale clip_ts into sequence time_base
    int64_t seq_ts = av_rescale_q(clip_ts, clip_tb, seq->audio_time_base);
    // rescale clip start_pts from video to audio time_base (in sequence)
    int64_t audio_start_ts = av_rescale_q(clip->start_pts, seq->video_time_base, seq->audio_time_base);
    // add clip starting point in sequence
    return audio_start_ts + seq_ts;
}

/**
 * Free entire sequence and all clips within
 * @param seq Sequence containing clips and clip data to be freed
 */
void free_sequence(Sequence *seq) {
    clearList(&(seq->clips));
}

/**
 * get sequence string
 * @param  seq Sequence to get data
 * @return     string allocated on heap, to be freed by caller
 */
char *print_sequence(Sequence *seq) {
    char *str = printVars(1, "\n==== Print Sequence ====\n");
    char buf[4096];
    sprintf(buf, "duration_pts: %ld\nduration_frames: %ld\n------\n",
            get_sequence_duration_pts(seq), get_sequence_duration(seq));
    catVars(&str, 1, buf);

    Node *currNode = seq->clips.head;
    for(int i = 0; currNode != NULL; i++) {
        Clip *c = (Clip *) currNode->data;
        sprintf(buf, "Clip [%d]\nurl: %s\nstart_pts: %ld\nend_pts: %ld\norig_start_pts: %ld\norig_end_pts: %ld\nvid_ctx: %p\n",
                i, c->vid_ctx->url, c->start_pts, c->end_pts, c->orig_start_pts, c->orig_end_pts, c->vid_ctx);
        catVars(&str, 1, buf);
        currNode = currNode->next;
    }
    return str;
}

/*************** EXAMPLE FUNCTIONS ***************/
/**
 * Test example showing how to read packets from sequence
 * @param seq Sequence to read
 */
void example_sequence_read_packets(Sequence *seq, bool close_clips_flag) {
    AVPacket pkt;
    while(sequence_read_packet(seq, &pkt, close_clips_flag) >= 0) {
        AVPacket orig_pkt = pkt;
        Clip *clip = get_current_clip(seq);
        if(clip == NULL) {
            printf("clip == NULL, printing raw pkt pts: %ld\n", pkt.pts);
        } else {
            printf("clip: %s | ", clip->vid_ctx->url);

            if(pkt.stream_index == clip->vid_ctx->video_stream_idx) {
                printf("Video packet! pts: %ld, dts: %ld, frame: %ld\n", pkt.pts, pkt.dts,
                        cov_video_pts(clip->vid_ctx, pkt.pts));
            } else if(pkt.stream_index == clip->vid_ctx->audio_stream_idx) {
                printf("Audio packet! pts: %ld, dts: %ld\n", pkt.pts, pkt.dts);
            }
        }
        av_packet_unref(&orig_pkt);
    }
}
