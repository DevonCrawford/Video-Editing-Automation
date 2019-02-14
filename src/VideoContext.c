#include "VideoContext.h"

AVStream *get_video_stream(VideoContext *vid_ctx) {
    return vid_ctx->fmt_ctx->streams[vid_ctx->video_stream_idx];
}

AVStream *get_audio_stream(VideoContext *vid_ctx) {
    return vid_ctx->fmt_ctx->streams[vid_ctx->audio_stream_idx];
}

void init_video_context(VideoContext *vid_ctx) {
    vid_ctx->fmt_ctx = NULL;
    vid_ctx->video_dec = NULL;
    vid_ctx->audio_dec = NULL;
    vid_ctx->video_dec_ctx = NULL;
    vid_ctx->audio_dec_ctx = NULL;
    vid_ctx->video_stream_idx = -1;
    vid_ctx->audio_stream_idx = -1;
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
    AVCodec *dec;
    AVCodecContext *dec_ctx;
    AVFormatContext *fmt_ctx = vid_ctx->fmt_ctx;
    AVDictionary *opts = NULL;
    int ret, refcount = 0;

    // finds the stream index given an AVMediaType (and gets decoder *dec on success)
    int stream_index = av_find_best_stream(fmt_ctx, type, -1, -1, &dec, 0);
    if(stream_index < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(type), fmt_ctx->url);
        return 1;
    } else {
        /* Create decoding context */
        dec_ctx = avcodec_alloc_context3(dec);
        if(!dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }
        // Fill the codec context based on the values from the supplied codec parameters.
        avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[stream_index]->codecpar);

        /* Init the decoders, with or without reference counting */
        av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
        if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        } else {
            // Attach decoder and decoder context to vid_ctx
            if(type == AVMEDIA_TYPE_VIDEO) {
                vid_ctx->video_dec = dec;
                vid_ctx->video_dec_ctx = dec_ctx;
                vid_ctx->video_stream_idx = stream_index;
            } else if(type == AVMEDIA_TYPE_AUDIO) {
                vid_ctx->audio_dec = dec;
                vid_ctx->audio_dec_ctx = dec_ctx;
                vid_ctx->audio_stream_idx = stream_index;
            }
            return 0;
        }
    }
}

/** free data inside VideoContext **/
void free_video_context(VideoContext *vid_ctx) {
    avcodec_free_context(&(vid_ctx->video_dec_ctx));
    avcodec_free_context(&(vid_ctx->audio_dec_ctx));
    avformat_close_input(&(vid_ctx->fmt_ctx));
}