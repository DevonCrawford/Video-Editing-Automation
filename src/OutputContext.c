#include "OutputContext.h"


/**
 * Initialize OutputContext structure
 * @param out_ctx OutputContext
 */
void init_video_output(OutputContext *out_ctx) {
    out_ctx->fmt_ctx = NULL;
    out_ctx->video_stream = NULL;
    out_ctx->audio_stream = NULL;
}

/**
 * Open format and file for video output
 * @param  out_ctx      OutputContext
 * @param  filename     name of output video file
 * @param  exampleClip  Clip to pull codec parameters for muxer (output codec)
 * @return          >= 0 on success
 */
int open_video_output(OutputContext *out_ctx, char *filename, Clip *exampleClip) {
    init_video_output(out_ctx);
    AVDictionary *opt = NULL;
    int ret;

    avformat_alloc_output_context2(&(out_ctx->fmt_ctx), NULL, NULL, filename);
    if (!(out_ctx->fmt_ctx)) {
        printf("Could not deduce output format from file extension: using MP4.\n");
        avformat_alloc_output_context2(&(out_ctx->fmt_ctx), NULL, "mp4", filename);
        if (!(out_ctx->fmt_ctx)) {
            fprintf(stderr, "Failed to allocate output context for file[%s]\n", filename);
            return -1;
        }
    }
    if(out_ctx->fmt_ctx->oformat->video_codec != AV_CODEC_ID_NONE) {
        printf("need to allocate output video codec.. creating video stream for muxing\n");
        // Create video stream for muxing
        out_ctx->video_stream = avformat_new_stream(out_ctx->fmt_ctx, NULL);
        if(!out_ctx->video_stream) {
            fprintf(stderr, "Could not allocate video stream");
            return -1;
        }
        /* copy the stream parameters to the muxer */
        ret = avcodec_parameters_from_context(out_ctx->video_stream->codecpar,
                                            exampleClip->vid_ctx->video_codec_ctx);
        out_ctx->video_stream->time_base = get_clip_video_time_base(exampleClip);
        printf("out_ctx->video_stream->time_base.num: %d\n", out_ctx->video_stream->time_base.num);
        printf("out_ctx->video_stream->time_base.den: %d\n", out_ctx->video_stream->time_base.den);
        if (ret < 0) {
            fprintf(stderr, "Could not copy the stream parameters\n");
            return -1;
        }
    } else {
        printf("out_ctx->fmt_ctx->oformat->video_codec == AV_CODEC_ID_NONE");
    }

    if(out_ctx->fmt_ctx->oformat->audio_codec != AV_CODEC_ID_NONE) {
        printf("need to allocate output audio codec.. creating audio stream for muxing\n");
        out_ctx->audio_stream = avformat_new_stream(out_ctx->fmt_ctx, NULL);
        if(!out_ctx->audio_stream) {
            fprintf(stderr, "Could not allocate audio stream");
            return -1;
        }
        /* copy the stream parameters to the muxer */
        ret = avcodec_parameters_from_context(out_ctx->audio_stream->codecpar,
                                            exampleClip->vid_ctx->audio_codec_ctx);
        out_ctx->audio_stream->time_base = get_clip_audio_time_base(exampleClip);
        if (ret < 0) {
            fprintf(stderr, "Could not copy the stream parameters\n");
            return -1;
        }
    } else {
        printf("out_ctx->fmt_ctx->oformat->audio_codec == AV_CODEC_ID_NONE");
    }

    /* open the output file, if needed */
    if(!(out_ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&(out_ctx->fmt_ctx->pb), filename, AVIO_FLAG_WRITE);
        if(ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", filename,
                    av_err2str(ret));
            return ret;
        }
    }

    ret = avformat_write_header(out_ctx->fmt_ctx, &opt);
    if(ret < 0) {
        fprintf(stderr, "open_video_output(): Error occurred when opening output file: %s\n",
                av_err2str(ret));
        close_video_output(out_ctx, false);
    }
    return ret;
}

/**
 * Open output file, write sequence packets and close the output file!
 * (this is an end to end solution)
 * @param  seq          Sequence containing clips to write to file
 * @param  filename     name of output video (extension determines container type)
 * @param  exampleClip  Clip to pull codec parameters for muxer (output codec)
 * @return          >= 0 on success
 */
int write_sequence(Sequence *seq, char *filename, Clip *exampleClip) {
    OutputContext out_ctx;
    int ret = open_video_output(&out_ctx, filename, exampleClip);
    if(ret < 0) {
        fprintf(stderr, "write_sequence(): Failed to open video output[%s]\n", filename);
        return ret;
    }

    ret = write_sequence_packets(&out_ctx, seq);
    if(ret < 0) {
        close_video_output(&out_ctx, true);
        return ret;
    }

    ret = close_video_output(&out_ctx, true);
    if(ret < 0) {
        fprintf(stderr, "write_sequence(): Failed to close video output[%s]\n", filename);
        return ret;
    }
    return 0;
}

/**
 * Write all sequence packets to an output file!
 * @param  fmt_ctx AVFormatContext already assumed to be allocated with open_video_output()
 * @param  seq     Sequence containing clips
 * @return         >= 0 on success
 */
int write_sequence_packets(OutputContext *out_ctx, Sequence *seq) {
    int ret;
    AVPacket pkt;
    while(sequence_read_packet(seq, &pkt, false) >= 0) {
        AVPacket orig_pkt = pkt;
        Clip *clip = get_current_clip(seq);
        if(clip == NULL) {
            printf("clip == NULL, printing raw pkt pts: %ld\n", pkt.pts);
            return -1;
        } else {
            printf("clip: %s | ", clip->url);
            // convert clip stream to corresponding output stream
            if(pkt.stream_index == clip->vid_ctx->video_stream_idx) {
                // set packet stream to video output
                pkt.stream_index = out_ctx->video_stream->index;
                printf("Video packet! pts: %ld, dts: %ld, frame: %ld, stream_idx: %d\n",
                    pkt.pts, pkt.dts, cov_video_pts(clip->vid_ctx, pkt.pts), pkt.stream_index);
            }
            else if(pkt.stream_index == clip->vid_ctx->audio_stream_idx) {
                // set packet stream to audio output
                pkt.stream_index = out_ctx->audio_stream->index;
                printf("Audio packet! pts: %ld, dts: %ld, stream_index: %d\n",
                                        pkt.pts, pkt.dts, pkt.stream_index);
            }
            // write packets in the same order as they come in
            ret = av_write_frame(out_ctx->fmt_ctx, &pkt);
            if (ret < 0) {
                fprintf(stderr, "Failed to write packet :(\n");
                return ret;
            }
        }
        av_packet_unref(&orig_pkt);
    }
    return 0;
}

/**
 * Close video output file
 * @param out_ctx
 * @param trailer_flag when true, trailer will attempt to be written
 * @return         >= 0 on success
 */
int close_video_output(OutputContext *out_ctx, bool trailer_flag) {
    int ret;
    if(trailer_flag) {
        ret = av_write_trailer(out_ctx->fmt_ctx);
        if(ret < 0) {
            fprintf(stderr, "Failed to write trailer [%s]\n", out_ctx->fmt_ctx->url);
            return ret;
        }
    }
    printf("wrote trailer successfully!!\n");
    if(!(out_ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        /* Close the output file */
        ret = avio_closep(&(out_ctx->fmt_ctx->pb));
        if(ret < 0) {
            fprintf(stderr, "Failed to close output file [%s]\n", out_ctx->fmt_ctx->url);
        }
    }
    /* free the stream */
    avformat_free_context(out_ctx->fmt_ctx);
    return ret;
}