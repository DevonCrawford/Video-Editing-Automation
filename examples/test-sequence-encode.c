/**
 * @file test-sequence-encode.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File testing the SequenceEncode and OutputContext API
 */

#include "OutputContext.h"

int main(int argc, char **argv) {
    if(argv[1] == NULL) {
        printf("Invalid usage. argv[1] should be filename for output\n");
        return -1;
    }
    Sequence seq;
    init_sequence(&seq, 30, 48000);

    Clip *clip1 = malloc(sizeof(Clip));
    Clip *clip2 = malloc(sizeof(Clip));
    Clip *clip3 = malloc(sizeof(Clip));
    init_clip(clip1, "test-resources/sequence/MVI_6529.MOV");
    init_clip(clip2, "test-resources/sequence/MVI_6530.MOV");
    init_clip(clip3, "test-resources/sequence/MVI_6531.MOV");

    open_clip(clip1);
    open_clip(clip2);
    open_clip(clip3);

    set_clip_bounds(clip1, 20, 30);
    set_clip_bounds(clip2, 60, 100);
    set_clip_bounds(clip3, 53, 100);

    // set_clip_bounds(clip1, 20, 100);
    // set_clip_bounds(clip2, 60, 400);
    // set_clip_bounds(clip3, 53, 500);

    sequence_append_clip(&seq, clip1);
    sequence_append_clip(&seq, clip2);
    sequence_append_clip(&seq, clip3);

    // sequence_seek(&seq, 11);

    OutputParameters op;
    VideoOutParams vp;
    AudioOutParams ap;
    set_video_out_params(&vp, clip1->vid_ctx->video_codec_ctx);
    vp.codec_id = AV_CODEC_ID_NONE;
    vp.bit_rate = -1;
    set_audio_out_params(&ap, clip1->vid_ctx->audio_codec_ctx);
    if(set_output_params(&op, argv[1], vp, ap) < 0) {
        return -1;
    }

    printf("\nREAD #1\n");
    printf("Start timing..\n");
    clock_t t;
    t = clock();

    write_sequence(&seq, &op);

    t = clock() - t;
    double time_taken = ((double)t)/(CLOCKS_PER_SEC/1000);
    printf("Completed in %fms.\n", time_taken);

    free_output_params(&op);
    free_sequence(&seq);
    return 0;
}