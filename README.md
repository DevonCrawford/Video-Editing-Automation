# Video Editing Automation
A toolkit of algorithms to automate the video editing process.

## How?
The goal is to write algorithmic building blocks that a human video editor can use
in place of their manual labour. This means a partially-autonomous editing
workflow, where the human must strategically use the toolkit of algorithms to
achieve their desired results. My rough personal guess is that when implemented
correctly, this could save about 80% of my time spent editing.

This project uses the [ffmpeg](https://ffmpeg.org/ffmpeg.html) library to handle much
of the heavy lifting of video processing, and intends to
build a library ontop of ffmpeg without changing the ffmpeg source code.
This project and its functions are their own isolated work (unless stated otherwise), however they do require linking with ffmpeg to operate correctly.
[View the ffmpeg source code](https://github.com/FFmpeg/FFmpeg) or
[Read the compilation guide](https://trac.ffmpeg.org/wiki/CompilationGuide/Generic)

If you would like to run this code you need to first install ffmpeg by compiling
the source (link above).

### Directory structure
- **src/** (source code without any main functions)
- **examples/** (examples with main functions that test and showcase src/ code)
- **include/** (header files for src/ code)
- **bin/** (build directory. object files and executables live here)

### Notes
By Default:
seek_video_pts() and seek_video() will seek to the nearest I-frame before the seek pts.
This must be done to collect valid packet data, because non I-frame packets do
not contain all the valid data within its frame. Therefore, the first few packets
from clip_read_packet() function will likely be before the clip->orig_start_pts.
They must be decoded as usual and dropped if they are before clip->orig_start_pts.

Read this for more information: https://stackoverflow.com/questions/39983025/how-to-read-any-frame-while-having-frame-number-using-ffmpeg-av-seek-frame/39990439

It would be possible to read packets and output them directly, but doing so while
seeking any frame(non I-frames) will result in glitches or freezes at the start
or end of clips. Reading, seeking then outputting direct packets only makes sense
when we seek to I-frames ONLY.

If we want precise seeking, then it really only makes sense to always decode packets and not use them directly. These functions will be implemented as clip_read_frame()
and sequence_read_frame(). They will build ontop of the read packet functions in
the same way, except they will also decode and throw away packets before orig_start_pts.

a work in progress by Devon Crawford