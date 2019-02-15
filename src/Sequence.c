#include "Sequence.h"

/**
 * Initialize new sequence and list of clips
 * @param  sequence Sequence is assumed to already be allocated memory
 * @return          >= 0 on success
 */
int init_sequence(Sequence *sequence, AVRational time_base, double fps) {
    if(sequence == NULL) {
        fprintf(stderr, "sequence is NULL, cannot initialize");
        return -1;
    }
    sequence->clips = initializeList(&list_print_clip, &list_delete_clip, &list_compare_clips);
    sequence->clips_iter = createIterator(sequence->clips);
    sequence->time_base = time_base;
    sequence->fps = fps;
    sequence->video_frame_duration = time_base.den / fps;
    return 0;
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
void sequence_add_clip_pts(Sequence *seq, Clip *clip, int start_pts) {
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
        int64_t start_pts = last->end_pts + seq->video_frame_duration;
        sequence_add_clip_pts(seq, clip, start_pts);
    }
}

/**
 * Convert sequence frame index to pts (presentation time stamp)
 * @param  seq         Sequence
 * @param  frame_index index of frame to convert
 * @return             pts representation of frame in sequence
 */
int64_t seq_frame_index_to_pts(Sequence *seq, int frame_index) {
    return seq->video_frame_duration * frame_index;
}

/**
 * Convert pts to frame index in sequence
 * @param  seq Sequence
 * @param  pts presentation time stamp to convert into frame index
 * @return     presentation time stamp representation of frame index
 */
int seq_pts_to_frame_index(Sequence *seq, int64_t pts) {
    return pts / seq->video_frame_duration;
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
    if(pts_diff >= 0 && seq_pts <= clip->end_pts) {
        // convert relative pts to clip time_base
        return av_rescale_q(pts_diff, seq->time_base, get_clip_video_time_base(clip));
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
    void *curr;
    ListIterator it = createIterator(seq->clips);
    while((curr = nextElement(&it)) != NULL) {
        Clip *clip = (Clip *) curr;
        int64_t clip_pts;
        // If clip is found at this frame index (in sequence)
        if((clip_pts = seq_frame_within_clip(seq, clip, frame_index)) > 0) {
            Clip *previous = (Clip *) seq->clips_iter.current->data;
            // If clips are different, close current and open next clip
            if(compare_clips(clip, previous) != 0) {
                close_clip(previous);
                open_clip(clip);
            }
            seq->clips_iter.current = it.current->previous;
            // seek to the correct pts within the clip!
            return seek_clip_pts(clip, clip_pts);
        }
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
 * @return     >= 0 on success, < 0 when reached end of sequence or error.
 */
int sequence_read_packet(Sequence *seq, AVPacket *pkt) {
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
        printf("End of clip[%s]\n", curr_clip->url);
        ret = seek_clip_pts(curr_clip, 0);                   // reset clip to start (for next time)
        if(ret < 0) {
            fprintf(stderr, "Failed to seek clip[%s] to its beginning\n", curr_clip->url);
            return ret;
        }
        close_clip(curr_clip);
        void *next = nextElement(&(seq->clips_iter));       // get next clip
        if(next == NULL) {
            // We're done reading all clips! (reset to start)
            seq->clips_iter.current = seq->clips.head;
            return -1;
        } else {
            // move onto next clip
            Clip *next_clip = (Clip *) next;
            open_clip(next_clip);
            return sequence_read_packet(seq, pkt);
        }
    } else {
        // valid packet down here
        *pkt = tmpPkt;
        return 0;
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
                                             seq->time_base);
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
 * Free entire sequence and all clips within
 * @param seq Sequence containing clips and clip data to be freed
 */
void free_sequence(Sequence *seq) {
    clearList(&(seq->clips));
}

/*************** EXAMPLE FUNCTIONS ***************/
/**
 * Test example showing how to read packets from sequence
 * @param seq Sequence to read
 */
void example_sequence_read_packets(Sequence *seq) {
    AVPacket pkt;
    while(sequence_read_packet(seq, &pkt) >= 0) {
        AVPacket orig_pkt = pkt;
        Clip *clip = get_current_clip(seq);
        if(clip == NULL) {
            printf("clip == NULL, printing raw pkt pts: %ld\n", pkt.pts);
        } else {
            printf("clip: %s | ", clip->url);

            if(pkt.stream_index == clip->vid_ctx->video_stream_idx) {
                printf("Video packet! pts: %ld, frame: %ld\n", pkt.pts,
                        cov_video_pts(clip->vid_ctx, pkt.pts));
            } else if(pkt.stream_index == clip->vid_ctx->audio_stream_idx) {
                printf("Audio packet! pts: %ld\n", pkt.pts);
            }
        }
        av_packet_unref(&orig_pkt);
    }
}
