/**
 * @file test-sequence.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File testing the Sequence API
 */

#include "Timebase.h"
#include "Sequence.h"
#include "OutputContext.h"

int main(int argc, char **argv) {
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

    set_clip_bounds(clip1, 20, 27);
    set_clip_bounds(clip2, 60, 68);
    set_clip_bounds(clip3, 53, 61);

    sequence_append_clip(&seq, clip1);
    sequence_append_clip(&seq, clip2);
    sequence_append_clip(&seq, clip3);



    char *str = print_sequence(&seq);
    printf("BEFORE CUT\n%s\n", str);
    free(str);
    str = NULL;

    int ret = cut_clip(&seq, 2);
    printf("cut return: %d\n", ret);

    str = print_sequence(&seq);
    printf("AFTER CUT\n%s\n", str);
    free(str);
    str = NULL;


    Clip *split = NULL;
    find_clip_at_index(&seq, 5, &split);
    // ret = sequence_ripple_delete_clip(&seq, split);

    int64_t cmp1 = list_compare_clips_sequential(clip1, clip2);
    int64_t cmp2 = list_compare_clips_sequential(clip1, clip1);
    int64_t cmp3 = list_compare_clips_sequential(clip2, clip1);
    int64_t cmp4 = list_compare_clips_sequential(clip3, clip1);
    int64_t cmp5 = list_compare_clips_sequential(clip1, split);
    int64_t cmp6 = list_compare_clips_sequential(split, clip1);
    int64_t cmp7 = list_compare_clips_sequential(split, split);
    //
    printf("cmp1: %ld\n", cmp1);
    printf("cmp2: %ld\n", cmp2);
    printf("cmp3: %ld\n", cmp3);
    printf("cmp4: %ld\n", cmp4);
    printf("cmp5: %ld\n", cmp5);
    printf("cmp6: %ld\n", cmp6);
    printf("cmp7: %ld\n", cmp7);
    //
    // str = print_sequence(&seq);
    // printf("AFTER DELETE CLIP\n%s\n", str);
    // free(str);
    // str = NULL;

    // printf("\nREAD #1\n");
    // printf("Start timing..\n");
    // clock_t t;
    // t = clock();
    // example_sequence_read_packets(&seq, false);
    // t = clock() - t;
    // double time_taken = ((double)t)/(CLOCKS_PER_SEC/1000);
    // printf("Completed in %fms.\n", time_taken);

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
