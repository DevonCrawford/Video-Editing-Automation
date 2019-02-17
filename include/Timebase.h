#ifndef _TIMEBASE_API_
#define _TIMEBASE_API_

/* Set our seek function to only seek to I-frames before our desired frame */
#define FFMPEG_SEEK_FLAG AVSEEK_FLAG_BACKWARD

#include "VideoContext.h"

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

#endif