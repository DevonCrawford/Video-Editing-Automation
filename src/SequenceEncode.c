/**
 * @file SequenceEncode.c
 * @author Devon Crawford
 * @date February 21, 2019
 * @brief File containing the source for SequenceEncode API:
 * These functions build ontop of SequenceDecode API to encode all frames
 * from a list of clips in a sequence
 */

#include "SequenceEncode.h"

 /**
  * Encode packets from a sequence
  * This function builds ontop of sequence_read_frame() in SequenceDecode.c
  * @param  oc           OutputContext already allocated with codecs
  * @param  seq_ts       Sequence containing clips to encode
  * @param  pkt          output encoded packet
  * @return      >= 0 on success, < 0 when reach EOF, end of clip boundary or error.
  */
 int sequence_encode_frame(OutputContext *oc, Sequence *seq, AVPacket *pkt) {
     if(oc->video.done_flush && oc->audio.done_flush) {
         return AVERROR_EOF;
     } else {
         if(oc->last_encoder_frame_type == AVMEDIA_TYPE_VIDEO || oc->video.flushing) {
             return seq_receive_enc_packet(oc, &(oc->video), seq, pkt);
         } else if(oc->last_encoder_frame_type == AVMEDIA_TYPE_AUDIO || oc->audio.flushing) {
             return seq_receive_enc_packet(oc, &(oc->audio), seq, pkt);
         } else {
             return seq_send_frame_to_encoder(oc, seq, pkt);
         }
     }
 }

 /*************** EXAMPLE FUNCTIONS ***************/
 /**
  * Test example showing how to encode frames from sequence
  * @param seq Sequence to encode
  */
  void example_sequence_encode_frames(OutputContext *oc, Sequence *seq) {
      AVPacket *pkt = av_packet_alloc();
      while(sequence_encode_frame(oc, seq, pkt) >= 0) {
          printf("sequence_encode_frame(): ");
          if(pkt->stream_index == oc->video.stream->index) {
              printf("Video packet! pts: %ld\n", pkt->pts);
          } else if(pkt->stream_index == oc->audio.stream->index) {
              printf("Audio packet! pts: %ld\n", pkt->pts);
          }
      }
      av_packet_free(&pkt);
  }

 /*************** INTERNAL FUNCTIONS ***************/
 /**
  * Receive an encoded packet given an output stream, and handle the return value
  * @param  oc   OutputContext
  * @param  os   OutputStream within OutputContext (video or audio)
  * @param  seq  Sequence being encoded
  * @param  pkt  output encoded packet
  * @return      >= 0 on success
  */
 int seq_receive_enc_packet(OutputContext *oc, OutputStream *os, Sequence *seq, AVPacket *pkt) {
     int ret = avcodec_receive_packet(os->codec_ctx, pkt);
     if(ret == 0) {
         // successfully received packet from encoder
         pkt->stream_index = os->stream->index;

         // rescale packet timestamp values from codec_ctx(sequence) to output stream timebase
         av_packet_rescale_ts(pkt, os->codec_ctx->time_base, os->stream->time_base);

         // printf("rescale %d/%d to %d/%d\n", seq->video_time_base.num, seq->video_time_base.den,
         // os->stream->time_base.num, os->stream->time_base.den);
         // rescale packet timestamp values from sequence/codec_ctx to output stream timebase
         // if(pkt->stream_index == oc->video.stream->index) {
         //     av_packet_rescale_ts(pkt, os->codec_ctx->time_base, os->stream->time_base);
         // } else if(pkt->stream_index == oc->audio.stream->index) {
         //     av_packet_rescale_ts(pkt, os->codec_ctx->time_base, os->stream->time_base);
         // }
         return ret;
     } else if(ret == AVERROR(EAGAIN)) {
         // output is not available in the current state - user must try to send input
         return seq_send_frame_to_encoder(oc, seq, pkt);
     } else if(ret == AVERROR_EOF) {
         // the encoder has been fully flushed, and there will be no more output frames
         os->flushing = false;
         os->done_flush = true;
         return sequence_encode_frame(oc, seq, pkt);
     } else {
         // legitimate encoding errors
         fprintf(stderr, "Legitmate encoding error when handling receive frame[%s]\n",
                             av_err2str(ret));
         return ret;
     }
 }

 void clear_frame_encoding_garbage(AVFrame *f) {
     f->key_frame = 0;
     f->pict_type = AV_PICTURE_TYPE_NONE;
     f->sample_aspect_ratio = (AVRational){0,1};
     f->coded_picture_number = 0;
     f->display_picture_number = 0;
     f->opaque = NULL;
     f->repeat_pict = 0;
     f->interlaced_frame = 0;
     f->side_data = NULL;
     f->nb_side_data = 0;
     f->flags = 0;
     f->decode_error_flags = 0;
     f->private_ref = NULL;
     f->opaque_ref = NULL;
 }

 /**
  * Send a frame to encoder. This function builds ontop of seq_read_frame().
  * @param  oc   OutputContext already allocated
  * @param  seq  Sequence to encode
  * @param  pkt  packet output
  * @return      >= 0 on success
  */
 int seq_send_frame_to_encoder(OutputContext *oc, Sequence *seq, AVPacket *pkt) {
     enum AVMediaType type;
     // read decoded frame from sequence
     int ret = sequence_read_frame(seq, oc->buffer_frame, &type, true);
     // no more frames or error
     if(ret < 0) {
         oc->last_encoder_frame_type = AVMEDIA_TYPE_NB;

         // if last clip(iterator will be reset to start), then flush video and audio streams
         if(seq->clips_iter.current == seq->clips.head) {
             ret = avcodec_send_frame(oc->video.codec_ctx, NULL);
             if(ret < 0) {
                 fprintf(stderr, "Failed to flush the video stream (%s)\n", av_err2str(ret));
                 return ret;
             }
             oc->video.flushing = true;

             ret = avcodec_send_frame(oc->audio.codec_ctx, NULL);
             if(ret < 0) {
                 fprintf(stderr, "Failed to flush the audio stream (%s)\n", av_err2str(ret));
                 return ret;
             }
             oc->audio.flushing = true;

             return sequence_encode_frame(oc, seq, pkt);   // try to receive a flushed packet out
         } else {
             return ret;
         }
     }
     // supply a raw video or audio frame to the encoder
     if(type == AVMEDIA_TYPE_VIDEO) {
         ret = avcodec_send_frame(oc->video.codec_ctx, oc->buffer_frame);
         return seq_handle_send_frame(oc, seq, type, ret, pkt);
     } else if(type == AVMEDIA_TYPE_AUDIO) {
         ret = avcodec_send_frame(oc->audio.codec_ctx, oc->buffer_frame);
         return seq_handle_send_frame(oc, seq, type, ret, pkt);
     } else {
         // this should never happen
         fprintf(stderr, "AVFrame type is invalid (must be AVMEDIA_TYPE_VIDEO or AVMEDIA_TYPE_AUDIO)\n");
         return -1;
     }
 }

 /**
  * Handle the return from avcodec_send_frame().
  * This function is to be used inside of seq_send_frame_to_encoder()
  * @param  oc   OutputContext already allocated
  * @param  seq  Sequence to encode
  * @param  type AVMediaType of frame that was sent
  * @param  ret  return from avcodec_send_frame()
  * @param  pkt  output packet
  * @return      >= 0 on success
  */
 int seq_handle_send_frame(OutputContext *oc, Sequence *seq, enum AVMediaType type, int ret, AVPacket *pkt) {
     if(ret == 0) {
         // successfully sent frame to encoder
         oc->last_encoder_frame_type = type;
         return sequence_encode_frame(oc, seq, pkt);    // receive the packet from encoder
     } else if(ret == AVERROR(EAGAIN)) {
         // input is not accepted in the current state - user must read output
         return sequence_encode_frame(oc, seq, pkt);
     } else {
         // legitimate encoding error
         fprintf(stderr, "Legitmate encoding error when handling send frame[%s]\n",
                             av_err2str(ret));
         return ret;
     }
 }