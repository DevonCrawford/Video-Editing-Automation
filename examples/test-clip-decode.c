/**
 * @file test-clip-decode.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File testing the ClipDecode API
 */

#include "ClipDecode.h"

int main(int argc, char **argv) {
    Clip *clip1 = malloc(sizeof(Clip));
    // Clip *clip2 = malloc(sizeof(Clip));
    init_clip(clip1, "test-resources/sequence/MVI_6529.MOV");
    // init_clip(clip2, "test-resources/sequence/MVI_6530.MOV");

    open_clip(clip1);
    // open_clip(clip2);

    set_clip_bounds(clip1, 20, 27);
    // set_clip_bounds(clip2, 60, 68);

    printf("example_clip_read_frames()\n\n");
    printf("\nREAD #1\n");
    printf("Start timing..\n");
    clock_t t;
    t = clock();
    example_clip_read_frames(clip1);
    t = clock() - t;
    double time_taken = ((double)t)/(CLOCKS_PER_SEC/1000);
    printf("Completed in %fms.\n", time_taken);

    printf("\nREAD #2\n");
    printf("Start timing..\n");
    t = clock();
    example_clip_read_frames(clip1);
    t = clock() - t;
    time_taken = ((double)t)/(CLOCKS_PER_SEC/1000);
    printf("Completed in %fms.\n", time_taken);

    free_clip(&clip1);
    return 0;
}