/**
 * @file Timebase.h
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the definition and usage for Timebase API:
 * Useful timebase conversions and seek functionality for VideoContext
 */

#ifndef _TIMEBASE_API_
#define _TIMEBASE_API_

/* Set our seek function to only seek to I-frames before our desired frame */
#define FFMPEG_SEEK_FLAG AVSEEK_FLAG_BACKWARD

#include "VideoContext.h"
#include <string.h>

/**
 * Check if AVRational is valid
 * @param  r AVRational to check
 * @return   true if invalid
 */
bool valid_rational(AVRational r);

/**
 * Convert video pts into frame index
 * @param  vid_ctx VideoContext
 * @param  pts     presentation timestamp of video frames
 * @return         >= 0 on success: frame index representation of pts param
 */
int64_t cov_video_pts(VideoContext *vid_ctx, int64_t pts);

/**
    Return >= 0 on success
*/
int64_t get_video_frame_pts(VideoContext *vid_ctx, int frameIndex);

int seek_video(VideoContext *vid_ctx, int frameIndex);

/**
    Return >= 0 on success
*/
int seek_video_pts(VideoContext *vid_ctx, int pts);

int64_t get_audio_frame_pts(VideoContext *vid_ctx, int frameIndex);

int64_t cov_video_to_audio_pts(VideoContext *vid_ctx, int videoFramePts);

/**
 * Print an AVRational struct
 * @param  tb timebase to print
 * @return    string allocated on heap. to be freed by caller
 */
char *print_time_base(AVRational *tb);

#endif