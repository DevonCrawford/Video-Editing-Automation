/**
 * @file OutputContext.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the source for OutputContext API:
 * These functions build ontop of SequenceEncode API to write encoded
 * frames to an output file.
 */

#include "OutputContext.h"

/**
 * Initialize output stream
 * @param os OutputStream
 */
void init_output_stream(OutputStream *os) {
    os->codec = NULL;
    os->codec_ctx = NULL;
    os->stream = NULL;
    os->flushing = false;
    os->done_flush = false;
}

/**
 * Initialize OutputContext structure
 * @param out_ctx       OutputContext
 */
void init_video_output(OutputContext *oc) {
    oc->fmt_ctx = NULL;
    init_output_stream(&(oc->video));
    init_output_stream(&(oc->audio));
    oc->buffer_frame = av_frame_alloc();
    oc->last_encoder_frame_type = AVMEDIA_TYPE_NB;
}

/**
 * Add a stream and allocate codecs for an OutputStream
 * @param  oc       OutputContext
 * @param  os       OutputStream within OutputContext
 * @param  codec_id id of codec to allocate
 * @return          >= 0 on success
 */
int add_stream(OutputContext *oc, OutputStream *os, enum AVCodecID codec_id) {
    // find the encoder
    os->codec = avcodec_find_encoder(codec_id);
    if (!(os->codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        return -1;
    }
    // allocate codec context
    os->codec_ctx = avcodec_alloc_context3(os->codec);
    if (!(os->codec_ctx)) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        return -1;
    }
    // Create stream for muxing
    os->stream = avformat_new_stream(oc->fmt_ctx, NULL);
    if(!(os->stream)) {
        fprintf(stderr, "Could not allocate stream");
        return -1;
    }
    os->stream->id = oc->fmt_ctx->nb_streams-1;
    return 0;
}

/**
 * Set video codec and muxer parameters from VideoOutParams
 * @param  oc OutputContext
 * @param  op VideoOutParams
 * @return    >= 0 on success
 */
int set_video_codec_params(OutputContext *oc, VideoOutParams *op) {
    OutputStream *os = &(oc->video);
    AVCodecContext *c = os->codec_ctx;
    c->width = op->width;
    c->height = op->height;
    c->pix_fmt = op->pix_fmt;
    if(op->bit_rate != -1) {
        c->bit_rate = op->bit_rate;
    }
    return 0;
}

/**
 * Set audio codec and muxer parameters from AudioOutParams
 * @param  oc OutputContext
 * @param  op AudioOutParams
 * @return    >= 0 on success
 */
int set_audio_codec_params(OutputContext *oc, AudioOutParams *op) {
    OutputStream *os = &(oc->audio);
    AVCodecContext *c = os->codec_ctx;
    c->sample_fmt = op->sample_fmt;
    c->bit_rate = op->bit_rate;
    c->sample_rate = op->sample_rate;
    c->channel_layout = op->channel_layout;
    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    return 0;
}

/**
 * copy params to muxer
 * @param  oc OutputContext
 * @param  os OutputStream (video or audio) within OutputContext
 * @param  c  AVCodecContext associated with video or audio stream.
 *              parameters from this will be copied to muxer
 * @return    >= 0 on success
 */
int set_muxer_params(OutputContext *oc, OutputStream *os, AVCodecContext *c) {
    /* Fill the parameters struct based on the values from the supplied codec context */
    int ret = avcodec_parameters_from_context(os->stream->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the codec parameters to the output stream (muxer)\n");
        return ret;
    }
    /* Some formats want stream headers to be separate. */
    if (oc->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        os->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    return 0;
}

int open_codec(OutputContext *oc, OutputStream *os) {
    /* Some formats want stream headers to be separate. */
    if (oc->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        os->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    // open video codec
    int ret = avcodec_open2(os->codec_ctx, os->codec, NULL);
    if(ret < 0) {
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
        return ret;
    }

    /* Fill the parameters struct based on the values from the supplied codec context */
    ret = avcodec_parameters_from_context(os->stream->codecpar, os->codec_ctx);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the codec parameters to the output stream (muxer)\n");
        return ret;
    }
    return 0;
}

/**
 * Open format and file for video output
 * @param  oc           OutputContext
 * @param  op           OutputParameters for video and audio codecs/muxers (filename as well)
 * @param  seq          Sequence to derive time_base for codec context
 * @return              >= 0 on success
 */
int open_video_output(OutputContext *oc, OutputParameters *op, Sequence *seq) {
    enum AVCodecID vid_codec_id, aud_codec_id;
    int ret;

    // Create AVFormatContext from input parameters
    avformat_alloc_output_context2(&(oc->fmt_ctx), NULL, NULL, op->filename);
    if (!(oc->fmt_ctx)) {
        printf("Could not deduce output format from file extension: using MP4.\n");
        avformat_alloc_output_context2(&(oc->fmt_ctx), NULL, "mp4", op->filename);
        if (!(oc->fmt_ctx)) {
            fprintf(stderr, "Failed to allocate output context for file[%s]\n", op->filename);
            return -1;
        }
    }
    // video codec id override from user defined params

    if(op->video.codec_id != AV_CODEC_ID_NONE) {
        printf("OVERRIDE VIDEO CODEC\n");
        oc->fmt_ctx->oformat->video_codec = op->video.codec_id;
    }
    // audio codec id override from user defined params
    if(op->audio.codec_id != AV_CODEC_ID_NONE) {
        printf("OVERRIDE AUDIO CODEC\n");
        oc->fmt_ctx->oformat->audio_codec = op->audio.codec_id;
    }
    vid_codec_id = oc->fmt_ctx->oformat->video_codec;
    aud_codec_id = oc->fmt_ctx->oformat->audio_codec;
    if(vid_codec_id != AV_CODEC_ID_NONE) {
        // create video stream
        ret = add_stream(oc, &(oc->video), vid_codec_id);
        if(ret < 0) {
            fprintf(stderr, "Failed to create video stream\n");
            return ret;
        }
        ret = set_video_codec_params(oc, &(op->video));
        if(ret < 0) {
            fprintf(stderr, "Failed to set video codec parameters\n");
            return ret;
        }
        // codec context timebase is derived from sequence timebase
        oc->video.codec_ctx->time_base = seq->video_time_base;

        ret = open_codec(oc, &(oc->video));
        if(ret < 0) {
            fprintf(stderr, "Failed to set open video codec\n");
            return ret;
        }
    } else {
        printf("out_ctx->fmt_ctx->oformat->video_codec == AV_CODEC_ID_NONE");
    }

    if(aud_codec_id != AV_CODEC_ID_NONE) {
        // create audio stream
        ret = add_stream(oc, &(oc->audio), aud_codec_id);
        if(ret < 0) {
            fprintf(stderr, "Failed to create audio stream\n");
            return ret;
        }
        ret = set_audio_codec_params(oc, &(op->audio));
        if(ret < 0) {
            fprintf(stderr, "Failed to set audio codec parameters\n");
            return ret;
        }
        // codec context time base is derived from sequence time base
        oc->audio.codec_ctx->time_base = seq->audio_time_base;

        // open audio codec
        ret = open_codec(oc, &(oc->audio));
        if(ret < 0) {
            fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
            return ret;
        }
    } else {
        printf("out_ctx->fmt_ctx->oformat->audio_codec == AV_CODEC_ID_NONE");
    }

    /* open the output file, if needed */
    if(!(oc->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&(oc->fmt_ctx->pb), op->filename, AVIO_FLAG_WRITE);
        if(ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", op->filename,
                    av_err2str(ret));
            return ret;
        }
    }

    ret = avformat_write_header(oc->fmt_ctx, NULL);
    if(ret < 0) {
        fprintf(stderr, "open_video_output(): Error occurred when opening output file: %s\n",
                av_err2str(ret));
        close_video_output(oc, false);
    }
    return ret;
}

/**
 * Open output file, write sequence packets and close the output file!
 * (this is an end to end solution)
 * @param  seq      Sequence containing clips to write to file
 * @param  op       OutputParameters for video and audio codec/muxers (and filename)
 * @return          >= 0 on success
 */
int write_sequence(Sequence *seq, OutputParameters *op) {
    OutputContext oc;
    init_video_output(&oc);
    int ret = open_video_output(&oc, op, seq);
    if(ret < 0) {
        fprintf(stderr, "write_sequence(): Failed to open video output[%s]\n", op->filename);
        return ret;
    }

    ret = write_sequence_frames(&oc, seq);
    if(ret < 0) {
        close_video_output(&oc, true);
        return ret;
    }

    ret = close_video_output(&oc, true);
    if(ret < 0) {
        fprintf(stderr, "write_sequence(): Failed to close video output[%s]\n", op->filename);
        return ret;
    }
    return 0;
}

void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;
    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

/**
 * Write entire sequence to an output file
 * @param  oc  OutputContext
 * @param  seq Sequence containing clips
 * @return     >= 0 on success
 */
int write_sequence_frames(OutputContext *oc, Sequence *seq) {
    int ret;
    AVPacket *pkt = av_packet_alloc();
    if(!pkt) {
        fprintf(stderr, "Could not allocate reusable packet for write sequence\n");
        return -1;
    }

    printf("Writing sequence to file[%s]..\n", oc->fmt_ctx->url);
    while(sequence_encode_frame(oc, seq, pkt) >= 0) {
        if(pkt->stream_index == oc->video.stream->index) {
            printf("Video Packet | ");
        } else if(pkt->stream_index == oc->audio.stream->index) {
            printf("Audio Packet | ");
        }
        log_packet(oc->fmt_ctx, pkt);
        // write the packet!
        ret = av_interleaved_write_frame(oc->fmt_ctx, pkt);
        if(ret < 0) {
            fprintf(stderr, "Failed to write encoded packet to file[%s]:%s\n",
                                        oc->fmt_ctx->url, av_err2str(ret));
            return ret;
        }
    }
    av_packet_free(&pkt);
    printf("Successfully wrote sequence to file[%s]\n", oc->fmt_ctx->url);
    return 0;
}

/**
 * Set OutputParameters given Video and Audio OutputParameters
 * @param op        OutputParameters to set filename, video and audio params
 * @param filename  name of output video file
 * @param vp        Video params
 * @param ap        Audio params
 * @return          >= 0 on success
 */
int set_output_params(OutputParameters *op, char *filename, VideoOutParams vp, AudioOutParams ap) {
    if(op == NULL || filename == NULL) {
        printf("set_output_params(): parameters cannot be NULL\n");
        return -1;
    }
    op->filename = malloc(sizeof(char) * (strlen(filename) + 1));
    strcpy(op->filename, filename);
    op->video = vp;
    op->audio = ap;
    return 0;
}

/**
 * Copy relevant codec context params (from decoder) into VideoOutParams struct (for encoding)
 * This can copy clip decoder settings to the encoder context
 * @param  op VideoOutParams
 * @param  c  AVCodecContext containing data (ideally from decoder)
 */
void set_video_out_params(VideoOutParams *op, AVCodecContext *c) {
    op->codec_id = c->codec_id;
    op->pix_fmt = c->pix_fmt;
    op->width = c->width;
    op->height = c->height;
    op->bit_rate = c->bit_rate;
}

/**
 * Copy relevant codec context params (from decoder) into AudioOutParams struct (for encoding)
 * @param  op AudioOutParams
 * @param  c  AVCodecContext containing data (ideally from decoder)
 */
void set_audio_out_params(AudioOutParams *op, AVCodecContext *c) {
    op->codec_id = c->codec_id;
    op->sample_fmt = c->sample_fmt;
    op->bit_rate = c->bit_rate;
    op->sample_rate = c->sample_rate;
    op->channel_layout = c->channel_layout;
}

/**
 * Free OutputParameters internal data
 * @param op OutputParameters to be freed
 */
void free_output_params(OutputParameters *op) {
    if(op->filename != NULL) {
        free(op->filename);
        op->filename = NULL;
    }
}

/**
 * Close the codec context within output stream
 * @param os OutputStream with open AVCodecContext
 */
void close_output_stream(OutputStream *os) {
    avcodec_free_context(&(os->codec_ctx));
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
        printf("wrote trailer successfully!!\n");
    }

    if(!(out_ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        /* Close the output file */
        ret = avio_closep(&(out_ctx->fmt_ctx->pb));
        if(ret < 0) {
            fprintf(stderr, "Failed to close output file [%s]\n", out_ctx->fmt_ctx->url);
        }
    }
    close_output_stream(&(out_ctx->video));     // close codecs
    close_output_stream(&(out_ctx->audio));
    av_frame_free(&(out_ctx->buffer_frame));    // free the buffer frame
    avformat_free_context(out_ctx->fmt_ctx);    // free the stream
    return ret;
}