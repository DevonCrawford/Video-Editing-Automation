/**
 * @file SequenceDecode.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the source for SequenceDecode API:
 * These functions build ontop of ClipDecode API to read all decoded frames
 * from a list of clips in a sequence
 */

#include "SequenceDecode.h"

/**
 * Read decoded frames from our editing sequence
 * @param  seq              Sequence with clips to be read
 * @param  frame            decoded output frame
 * @param  frame_type       type of output frame
 * @param  close_clips_flag if true, close clips after read
 * @return                  >= 0 on success (returned a frame)
 *                          < 0 when reached end of sequence or error
 */
int sequence_read_frame(Sequence *seq, AVFrame *frame, enum AVMediaType *frame_type, bool close_clips_flag) {
    Node *currNode = seq->clips_iter.current;
    if(currNode == NULL) {
        printf("sequence_read_frame() currNode == NULL\n");
        return -1;
    }
    Clip *curr_clip = (Clip *) currNode->data;    // current clip
    int ret;
    // If VideoContext was used by another clip, and is now out of bounds of current clip. Reset seek
    if(is_vc_out_bounds(curr_clip)) {
        ret = seek_clip_pts(curr_clip, 0);
        if(ret < 0) {
            return ret;
        }
    }
    ret = clip_read_frame(curr_clip, frame, frame_type);
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
            if(seq->clips.head != NULL) {
                open_clip((Clip *)(seq->clips.head->data));
            }
            ret = sequence_seek(seq, 0);
            if(ret < 0) {
                fprintf(stderr, "sequence_read_frame() error: Failed to seek to the start of sequence\n");
                return ret;
            }
            return -1;
        } else {
            // move onto next clip
            Clip *next_clip = (Clip *) ((Node *)next)->data;
            open_clip(next_clip);
            return sequence_read_frame(seq, frame, frame_type, close_clips_flag);
        }
    } else {
        clear_frame_decoding_garbage(frame);
        // Convert original packet timestamps into sequence timestamps
        if(*frame_type == AVMEDIA_TYPE_VIDEO) {
            // set first frame to be an I frame
            if(curr_clip->frame_index == 1) {
                frame->key_frame = 1;
                frame->pict_type = AV_PICTURE_TYPE_I;
            }
            frame->pts = video_pkt_to_seq_ts(seq, curr_clip, frame->pts);
            // frame->pkt_dts = video_pkt_to_seq_ts(seq, curr_clip, frame->pkt_dts);
        } else if(*frame_type == AVMEDIA_TYPE_AUDIO) {
            frame->pts = audio_pkt_to_seq_ts(seq, curr_clip, frame->pts);
            // frame->pkt_dts = audio_pkt_to_seq_ts(seq, curr_clip, frame->pkt_dts);
        }
        return 0;
    }
}

/**
 * Clear fields on AVFrame from decoding
 * @param f AVFrame to initialize
 */
void clear_frame_decoding_garbage(AVFrame *f) {
    f->key_frame = 0;
    f->pict_type = AV_PICTURE_TYPE_NONE;
    f->sample_aspect_ratio = (AVRational){0,1};
    f->coded_picture_number = 0;
    f->display_picture_number = 0;
    f->opaque = NULL;
    f->repeat_pict = 0;
    f->interlaced_frame = 0;
    f->side_data = NULL;
    f->nb_side_data = 0;
    f->flags = 0;
    f->decode_error_flags = 0;
    f->private_ref = NULL;
    f->opaque_ref = NULL;
}

/*************** EXAMPLE FUNCTIONS ***************/
/**
 * Test example showing how to read frames from sequence
 * @param seq Sequence to read
 */
 int example_sequence_read_frames(Sequence *seq, bool close_clips_flag) {
     enum AVMediaType type;
     AVFrame *frame = av_frame_alloc();
     if(!frame) {
         fprintf(stderr, "Could not allocate frame\n");
         return AVERROR(ENOMEM);
     }
     while(sequence_read_frame(seq, frame, &type, close_clips_flag) >= 0) {
         Clip *clip = get_current_clip(seq);
         if(clip == NULL) {
             printf("clip == NULL, printing raw frame pts: %ld\n", frame->pts);
         } else {
             printf("clip: %s | ", clip->vid_ctx->url);
             if(type == AVMEDIA_TYPE_VIDEO) {
                 printf("Video frame! pts: %ld, frame: %ld\n", frame->pts,
                         cov_video_pts(clip->vid_ctx, frame->pts));
             } else if(type == AVMEDIA_TYPE_AUDIO) {
                 printf("Audio frame! pts: %ld\n", frame->pts);
             }
         }
     }
     av_frame_free(&frame);
     return 0;
 }