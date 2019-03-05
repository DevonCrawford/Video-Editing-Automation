/**
 * @file test-clip-encode.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File testing the ClipEncode API
 */

#include "ClipEncode.h"

int main(int argc, char **argv) {
    int ret = 0;
    OutputContext oc;
    init_video_output(&oc);

    Sequence seq;
    init_sequence(&seq, 30, 48000);

    Clip *clip1 = malloc(sizeof(Clip));
    // Clip *clip2 = malloc(sizeof(Clip));
    init_clip(clip1, "test-resources/sequence/MVI_6529.MOV");
    // init_clip(clip2, "test-resources/sequence/MVI_6530.MOV");

    open_clip(clip1);
    // open_clip(clip2);

    set_clip_bounds(clip1, 20, 27);
    // set_clip_bounds(clip2, 60, 68);

    AVCodecParameters *video_par = get_clip_video_params(clip1);
    AVCodecParameters *audio_par = get_clip_audio_params(clip1);

    OutputParameters op;
    VideoOutParams vp;
    AudioOutParams ap;
    set_video_out_params(&vp, clip1->vid_ctx->video_codec_ctx);
    set_audio_out_params(&ap, clip1->vid_ctx->audio_codec_ctx);
    if(set_output_params(&op, "test-resources/sequence/out.mov", vp, ap) < 0) {
        return -1;
    }

    ret = open_video_output(&oc, &op, &seq);
    if(ret < 0) {
        printf("Failed to open video output\n");
        close_video_output(&oc, false);
        free_clip(&clip1);
        return -1;
    }

    printf("example_clip_encode_frames()\n\n");
    printf("WRITE #1\n");
    printf("Start timing..\n");
    clock_t t;
    t = clock();

    // example_clip_read_frames(clip1);
    example_clip_encode_frames(&oc, clip1);

    t = clock() - t;
    double time_taken = ((double)t)/(CLOCKS_PER_SEC/1000);
    printf("Completed in %fms.\n", time_taken);

    avcodec_parameters_free(&video_par);
    avcodec_parameters_free(&audio_par);

    close_video_output(&oc, true);
    free_clip(&clip1);
    free_sequence(&seq);

    return ret;
}