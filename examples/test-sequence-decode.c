/**
 * @file test-sequence-decode.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File testing the SequenceDecode API
 */

#include "SequenceDecode.h"

int main(int argc, char **argv) {
    Sequence seq;
    init_sequence(&seq, 29.97, 48000);

    Clip *clip1 = malloc(sizeof(Clip));
    Clip *clip2 = malloc(sizeof(Clip));
    Clip *clip3 = malloc(sizeof(Clip));
    init_clip(clip1, "test-resources/sequence/MVI_6529.MOV");
    init_clip(clip2, "test-resources/sequence/MVI_6530.MOV");
    init_clip(clip3, "test-resources/sequence/MVI_6531.MOV");

    open_clip(clip1);
    open_clip(clip2);
    open_clip(clip3);

    set_clip_bounds(clip1, 20, 35);
    set_clip_bounds(clip2, 60, 68);
    set_clip_bounds(clip3, 53, 61);

    sequence_append_clip(&seq, clip1);
    sequence_append_clip(&seq, clip2);
    sequence_append_clip(&seq, clip3);

    sequence_seek(&seq, 11);

    printf("\nREAD #1\n");
    printf("Start timing..\n");
    clock_t t;
    t = clock();
    example_sequence_read_frames(&seq, false);
    t = clock() - t;
    double time_taken = ((double)t)/(CLOCKS_PER_SEC/1000);
    printf("Completed in %fms.\n", time_taken);

    // printf("\nWRITE #1\n");
    // printf("Start timing..\n");
    // clock_t t2 = clock();
    //
    // // write sequence to output file!
    // write_sequence(&seq, "test-resources/sequence/out.mov", clip1);
    //
    // t2 = clock() - t2;
    // double timew1 = ((double)t2)/(CLOCKS_PER_SEC/1000);
    // printf("Completed in %fms.\n", timew1);

    // printf("\nREAD #2!!!\n");
    // printf("Start timing..\n");
    // t = clock();
    // example_sequence_read_packets(&seq, true);
    // t = clock() - t;
    // time_taken = ((double)t)/(CLOCKS_PER_SEC/1000);
    // printf("Completed in %fms.\n", time_taken);

    // printf("\nREAD #3!!!\n");
    // printf("Start timing..\n");
    // t = clock();
    // sequence_seek(&seq, 11);
    // example_sequence_read_packets(&seq, true);
    // t = clock() - t;
    // time_taken = ((double)t)/(CLOCKS_PER_SEC/1000);
    // printf("Completed in %fms.\n", time_taken);

    free_sequence(&seq);
    return 0;
}