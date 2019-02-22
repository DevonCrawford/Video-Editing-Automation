/**
 * @file VideoContext.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the source for VideoContext API:
 * The primary data structure for raw media files, videos just as they
 * are on disk.
 * Higher level structures such as Clip build ontop of this.
 */

#include "VideoContext.h"

AVStream *get_video_stream(VideoContext *vid_ctx) {
    int index = vid_ctx->video_stream_idx;
    if(index == -1) {
        return NULL;
    }
    return vid_ctx->fmt_ctx->streams[index];
}

AVStream *get_audio_stream(VideoContext *vid_ctx) {
    int index = vid_ctx->audio_stream_idx;
    if(index == -1) {
        return NULL;
    }
    return vid_ctx->fmt_ctx->streams[index];
}

AVRational get_video_time_base(VideoContext *vid_ctx) {
    return get_video_stream(vid_ctx)->time_base;
}

AVRational get_audio_time_base(VideoContext *vid_ctx) {
    return get_audio_stream(vid_ctx)->time_base;
}

void init_video_context(VideoContext *vid_ctx) {
    vid_ctx->fmt_ctx = NULL;
    vid_ctx->video_codec = NULL;
    vid_ctx->audio_codec = NULL;
    vid_ctx->video_codec_ctx = NULL;
    vid_ctx->audio_codec_ctx = NULL;
    vid_ctx->video_stream_idx = -1;
    vid_ctx->audio_stream_idx = -1;
    vid_ctx->last_decoder_packet_stream = DEC_STREAM_NONE;
}

/*
    out @param vid_ctx - allocated context containing format, streams and codecs
    in @param filename - name of video file
    Return >= 0 if OK, < 0 on fail
*/
int open_video(VideoContext *vid_ctx, char *filename) {
    int ret;
    if((ret = open_format_context(vid_ctx, filename)) < 0) {
        return ret;
    }
    int types[VIDEO_CONTEXT_STREAM_TYPES_LEN] = {
        AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO
    };
    // open stream and codec context for each stream type defined above^
    for(int i = 0; i < VIDEO_CONTEXT_STREAM_TYPES_LEN; i++) {
        if((ret = open_codec_context(vid_ctx, types[i])) < 0) {
            return ret;
        }
        if(ret == 1 && types[i] == AVMEDIA_TYPE_VIDEO) {
            fprintf(stderr, "Video stream is required and could not be found");
            return -1;
        }
    }
    return 0;
}

/* Return >=0 if OK, < 0 on fail */
int open_format_context(VideoContext *vid_ctx, char *filename) {
    init_video_context(vid_ctx);
    // open input file and allocate format context
    if(avformat_open_input(&(vid_ctx->fmt_ctx), filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", filename);
        return -1;
    }
    // retrieve stream information
    if(avformat_find_stream_info(vid_ctx->fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information for file [%s]\n", filename);
        return -1;
    }
    return 0;
}

/**
    Finds a stream and its associated codec
    Call this function after open_format_context() - for each AVMediaType
    Returns >= 0 if OK, < 0 on fail
*/
int open_codec_context(VideoContext *vid_ctx, enum AVMediaType type) {
    if(type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) {
        fprintf(stderr, "Unsupported type '%s'. Video_Context does not support this stream\n",
                av_get_media_type_string(type));
        return -1;
    }
    AVCodec *codec;
    AVCodecContext *codec_ctx;
    AVFormatContext *fmt_ctx = vid_ctx->fmt_ctx;
    AVDictionary *opts = NULL;
    int ret, refcount = 0;

    // finds the stream index given an AVMediaType (and gets codec on success)
    int stream_index = av_find_best_stream(fmt_ctx, type, -1, -1, &codec, 0);
    if(stream_index < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(type), fmt_ctx->url);
        return 1;
    } else {
        /* Create codec context */
        codec_ctx = avcodec_alloc_context3(codec);
        if(!codec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }
        // Fill the codec context based on the values from the supplied codec parameters.
        avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[stream_index]->codecpar);

        /* Init the codec context, with or without reference counting */
        av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
        if ((ret = avcodec_open2(codec_ctx, codec, &opts)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        } else {
            codec_ctx->time_base = fmt_ctx->streams[stream_index]->time_base;
            // Attach codec and codec context to vid_ctx
            if(type == AVMEDIA_TYPE_VIDEO) {
                vid_ctx->video_codec = codec;
                vid_ctx->video_codec_ctx = codec_ctx;
                vid_ctx->video_stream_idx = stream_index;
            } else if(type == AVMEDIA_TYPE_AUDIO) {
                vid_ctx->audio_codec = codec;
                vid_ctx->audio_codec_ctx = codec_ctx;
                vid_ctx->audio_stream_idx = stream_index;
            }
            return 0;
        }
    }
}

/** free data inside VideoContext **/
void free_video_context(VideoContext *vid_ctx) {
    avcodec_free_context(&(vid_ctx->video_codec_ctx));
    avcodec_free_context(&(vid_ctx->audio_codec_ctx));
    avformat_close_input(&(vid_ctx->fmt_ctx));
}

/**
 * Print most data within an AVCodecContext
 * @param  c AVCodecContext with data
 * @return   char * allocated on heap, it is the callers responsibility to free
 */
char *print_codec_context(AVCodecContext *c) {
    char buf[1024], channel_layout[100];
    buf[0] = 0;
    channel_layout[0] = 0;

    // https://www.ffmpeg.org/doxygen/trunk/group__channel__mask__c.html
    av_get_channel_layout_string(channel_layout, 100, av_get_channel_layout_nb_channels(c->channel_layout), c->channel_layout);

    sprintf(buf, "codec_type: %s\ncodec_id: %s\ncodec_tag: %d\nbit_rate: %ld\nbit_rate_tolerance: %d\nglobal_quality: %d\ncompression_level: %d\nflags: %d\nflags2: %d\nextradata_size: %d\ntime_base: %d/%d\nticks_per_frame: %d\ndelay: %d\nwidth: %d\nheight: %d\ncoded_width: %d\ncoded_height: %d\ngop_size: %d\npix_fmt: %s\ncolorspace: %s\ncolor_range: %s\nchroma_sample_location: %s\nslices: %d\nfield_order: %d\nsample_rate: %d\nchannels: %d\nsample_fmt: %s\nframe_size: %d\nframe_number: %d\nblock_align: %d\ncutoff: %d\nchannel_layout: %s\nrequest_channel_layout: %ld\n",
    av_get_media_type_string(c->codec_type), avcodec_get_name(c->codec_id), c->codec_tag, c->bit_rate, c->bit_rate_tolerance, c->global_quality, c->compression_level, c->flags, c->flags2, c->extradata_size, c->time_base.num, c->time_base.den, c->ticks_per_frame, c->delay, c->width, c->height, c->coded_width, c->coded_height, c->gop_size, av_get_pix_fmt_name(c->pix_fmt), av_get_colorspace_name(c->colorspace), av_color_range_name(c->color_range), av_chroma_location_name(c->chroma_sample_location), c->slices, c->field_order, c->sample_rate, c->channels, av_get_sample_fmt_name(c->sample_fmt), c->frame_size, c->frame_number, c->block_align, c->cutoff, channel_layout, c->request_channel_layout);
    char *str = malloc(sizeof(char) * (strlen(buf) + 1));
    strcpy(str, buf);
    return str;
}