/**
 * @file Sequence.h
 * @author Devon Crawford
 * @date February 14, 2019
 * @brief File containing the definitions and usage of the Sequence API:
 * A sequence is a list of clips in the editing timeline
 */

#ifndef _SEQUENCE_API_
#define _SEQUENCE_API_

#define SEQ_VIDEO_FRAME_DURATION 1000

#include "Clip.h"
#include "LinkedListAPI.h"
#include "Util.h"

/**
 * Define the Sequence structure.
 * A Sequence is a list of clips in a realtime video editor
 */
typedef struct Sequence {
    /*
        LinkedList of Clips in order of clip->pts
        Data type: struct Clip
     */
    List clips;
    /*
        ListIterator object used for iterating and seeking clips
     */
    ListIterator clips_iter;
    /*
        This is the fundamental unit of time (in seconds) in terms of which frame timestamps are represented.
     */
    AVRational video_time_base, audio_time_base;
    /*
        Video frames per second.
     */
    double fps;
    /*
        Duration of a single video frame in time_base units.
        Calculation = time_base.den / fps
     */
    int video_frame_duration;

    /*
        Current location of the seek pointer within the sequence
        (We track this by video packets seen and seek usage)
     */
    int64_t current_frame_idx;

    /*
        Current clip index
     */
    int current_clip_idx;
} Sequence;

/**
 * Initialize new sequence and list of clips
 * @param  sequence     Sequence is assumed to already be allocated memory
 * @param  fps          frames per second for the video stream. Video time base = 1/(fps*1000)
 * @param  sample_rate  sample rate of the audio stream. Audio time base = 1/sample_rate
 * @return              >= 0 on success
 */
int init_sequence(Sequence *seq, double fps, int sample_rate);

/**
 * Initialize a new sequence, list of clips along with a custom compare function (for insertSorted)
 * @param  seq         Sequence to initialize
 * @param  fps         frames per second
 * @param  sample_rate sample_rate of the audio stream
 * @param  compareFunc custom compare function used in sorting and searching
 * @return             >= 0 on success
 */
int init_sequence_cmp(Sequence *seq, double fps, int sample_rate, int (*compareFunc)(const void* first,const void* second));

/**
 * Allocate a clip within a sequence and use a reference to the same videoContext
 * for clips with the same url (filename)
 * @param  seq Sequence where clip will be added (used to find videoContext)
 * @param  url filename of clip to create
 * @return     >= 0 on success
 */
Clip *seq_alloc_clip(Sequence *seq, char *url);

/**
 * Find clip with the search url in a sequence
 * @param  seq Sequence containing clips to be searched
 * @param  url search key
 * @return     NULL on fail, not NULL on success
 */
Clip *find_clip(Sequence *seq, char *url);

/**
 * Get duration of sequence in frames (defined by fps)
 * @param  seq Sequence
 * @return     >= 0 on success
 */
int64_t get_sequence_duration(Sequence *seq);

/**
 * Get duration of sequence in pts (sequence timebase)
 * @param  seq Sequence
 * @return     >= 0 on success
 */
int64_t get_sequence_duration_pts(Sequence *seq);

/**
 * Insert Clip in sequence in sorted clip->pts order
 * @param  sequence Sequence containing a list of clips
 * @param  clip     Clip to be added into the sequence list of clips
 * @param  start_frame_index
 * @return          >= 0 on success
 */
void sequence_add_clip(Sequence *seq, Clip *clip, int start_frame_index);

/**
 * Insert Clip in sequence in sorted clip->pts order
 * @param  sequence Sequence containing a list of clips
 * @param  clip     Clip to be added into the sequence list of clips
 * @param  start_pts
 * @return          >= 0 on success
 */
void sequence_add_clip_pts(Sequence *seq, Clip *clip, int64_t start_pts);

/**
 * Add clip to the end of sequence
 * @param seq  Sequence to insert clip
 * @param clip Clip to be inserted into end of sequence
 */
void sequence_append_clip(Sequence *seq, Clip *clip);

/**
 * Insert clip sorted by:
 * 1. Date & time of file
 * 2. clip->orig_start_pts
 * This function will generate the sequence pts for the clip (clip->start_pts and clip->end_pts)
 * @param  seq  Sequence
 * @param  clip Clip to insert
 * @return      >= 0 on success
 */
int sequence_insert_clip_sorted(Sequence *seq, Clip *clip);

/**
 * Shift clips sequence pts to after the current node (insert clip function)
 * @param  seq          Sequence
 * @param  curr_node    current clip which should shift all following nodes
 * @return      >= 0 on success
 */
int shift_clips_after(Sequence *seq, Node *curr_node);

/**
 * Delete a clip from a sequence and move all following clips forward
 * @param  seq  Sequence
 * @param  clip Clip to delete
 * @return      >= 0 on success
 */
int sequence_ripple_delete_clip(Sequence *seq, Clip *clip);

/**
 * Convert sequence frame index to pts (presentation time stamp)
 * @param  seq         Sequence
 * @param  frame_index index of frame to convert
 * @return             pts representation of frame in sequence
 */
int64_t seq_frame_index_to_pts(Sequence *seq, int frame_index);

/**
 * Convert pts to frame index in sequence
 * @param  seq Sequence
 * @param  pts presentation time stamp to convert into frame index
 * @return     presentation time stamp representation of frame index
 */
int seq_pts_to_frame_index(Sequence *seq, int64_t pts);

/**
 * Cut a clip within a sequence, splitting the clip in two
 * @param  seq         Sequence
 * @param  frame_index index of frame in sequence (clip must lie at this point)
 * @return             >= 0 on success
 */
int cut_clip(Sequence *seq, int frame_index);

/**
 * Iterate all sequence clips to find the clip that contains this frame_index in sequence
 * @param  seq         Sequence
 * @param  frame_index index of frame in sequence
 * @param  found_clip  output clip if found. If not found this will be NULL
 * @return             >= 0 on success, -1 on fail.
 *                      The success number will be the pts relative to the clip,
 *                      and clip timebase (where zero represents clip->orig_start_pts)
 */
int64_t find_clip_at_index(Sequence *seq, int frame_index, Clip **found_clip);

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
int64_t seq_frame_within_clip(Sequence *seq, Clip *clip, int frame_index);

/**
 * Seek to an exact frame within the sequence (and all the clips within it)!
 * @param  seq         Sequence containing clips
 * @param  frame_index index in sequence, will be used to find clip and the clip frame
 * @return             >= 0 on success
 */
int sequence_seek(Sequence *seq, int frame_index);

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
 *          maybe the most CPU efficient solution is opening the clips before usage of this function (and keeping them open for a while)
 * @return     >= 0 on success, < 0 when reached end of sequence or error.
 */
int sequence_read_packet(Sequence *seq, AVPacket *pkt, bool close_clips_flag);

/**
 * Sets the start_pts of a clip in sequence
 * @param  seq               Sequence containing clip
 * @param  clip              Clip to set start_pts
 * @param  start_frame_index frame index in sequence to start the clip
 * @return                   >= 0 on success
 */
int move_clip(Sequence *seq, Clip *clip, int start_frame_index);

/**
 * Sets the start_pts of a clip in sequence
 * @param  seq               Sequence containing clip
 * @param  clip              Clip to set start_pts
 * @param  start_pts         pts in sequence to start the clip
 * @return                   >= 0 on success
 */
void move_clip_pts(Sequence *seq, Clip *clip, int64_t start_pts);

/**
 * Get current clip from sequence (seek position for next read)
 * @param  seq Sequence containing clips
 * @return     Clip that is currently being read
 */
Clip *get_current_clip(Sequence *seq);

/**
 * Convert a raw packet timestamp into a sequence timestamp
 * @param  seq          Sequence containing clip
 * @param  clip         Clip within sequence
 * @param  orig_pkt_ts  timestamp from AVPacket read directly from file (or clip_read_packet())
 * @return              timestamp representation of the video packet in the editing sequence!
 */
int64_t video_pkt_to_seq_ts(Sequence *seq, Clip *clip, int64_t orig_pkt_ts);

/**
 * Convert a raw packet timestamp into a sequence timestamp
 * @param  seq          Sequence containing clip
 * @param  clip         Clip within sequence
 * @param  orig_pkt_ts  timestamp from AVPacket read directly from file (or clip_read_packet())
 * @return              timestamp representation of the audio packet in the editing sequence!
 */
int64_t audio_pkt_to_seq_ts(Sequence *seq, Clip *clip, int64_t orig_pkt_ts);

/**
 * Free entire sequence and all clips within
 * @param seq Sequence containing clips and clip data to be freed
 */
void free_sequence(Sequence *seq);

/**
 * get sequence string
 * @param  seq Sequence to get data
 * @return     string allocated on heap, to be freed by caller
 */
char *print_sequence(Sequence *seq);

/*************** EXAMPLE FUNCTIONS ***************/
/**
 * Test example showing how to read packets from sequence
 * @param seq Sequence to read
 */
void example_sequence_read_packets(Sequence *seq, bool close_clips_flag);

#endif