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

a work in progress by Devon Crawford