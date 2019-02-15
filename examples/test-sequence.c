#include "Timebase.h"
#include "Sequence.h"

int main(int argc, char **argv) {
    Sequence seq;
    init_sequence(&seq, (AVRational){1, 30000}, 29.97);

    Clip *clip1 = malloc(sizeof(Clip));
    Clip *clip2 = malloc(sizeof(Clip));
    Clip *clip3 = malloc(sizeof(Clip));
    init_clip(clip1, "test-resources/sequence/MVI_6529.MOV");
    init_clip(clip2, "test-resources/sequence/MVI_6530.MOV");
    init_clip(clip3, "test-resources/sequence/MVI_6531.MOV");

    open_clip(clip1);
    open_clip(clip2);
    open_clip(clip3);

    set_clip_bounds(clip1, 20, 26);
    set_clip_bounds(clip2, 60, 67);
    set_clip_bounds(clip3, 53, 163);

    // set_clip_bounds(&clip1, 10, 75);
    // set_clip_bounds(&clip2, 60, 120);
    // set_clip_bounds(&clip3, 53, 165);
    sequence_append_clip(&seq, clip1);
    sequence_append_clip(&seq, clip2);
    sequence_append_clip(&seq, clip3);

    clock_t t;
    t = clock();
    example_sequence_read_packets(&seq);
    t = clock() - t;
    double time_taken = ((double)t)/(CLOCKS_PER_SEC/1000);
    printf("Completed in %fms.\n", time_taken);

    free_sequence(&seq);
    return 0;
}



// int cut_range(AVFormatContext *fmt_ctx, int64_t startFrame, int64_t endFrame) {
//     return cut(filename, startFrame, endFrame-startFrame);
// }
//
// int cut(AVFormatContext *fmt_ctx, int64_t startFrame, int64_t duration) {
//
// }