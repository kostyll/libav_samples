#include "general.h"
#include "transcode.h"

void clean_up_packets(
    AVPacket *pkt1_for_free,
    AVPacket *pkt2_for_free,
    AVPacket *pkt3_for_free
){
    if (pkt1_for_free != NULL) av_free_packet(pkt1_for_free);
    if (pkt2_for_free != NULL) av_free_packet(pkt2_for_free);
    if (pkt3_for_free != NULL) av_free_packet(pkt3_for_free);
}

int native_process_handler(
    TranscodingFunc func,
    TranscodingContext * tctx,
    InputSource * source,
    Output * output,
    AVPacket *pkt1_for_free,
    AVPacket *pkt2_for_free,
    AVPacket *pkt3_for_free
){
    int ret;
    if (func == NULL)
        return 1;
    ret = (*func)(source, output, tctx);
    if (ret <= 0){
        clean_up_packets(
            pkt1_for_free,
            pkt2_for_free,
            pkt3_for_free
        );
    }
    return ret;
};

ProcessHandler current_process_handler = &native_process_handler;

int set_process_handler(
    ProcessHandler func_ptr
){
    if (func_ptr == NULL) current_process_handler = &native_process_handler;
        else current_process_handler = func_ptr;
    return 0;
}


int process_video_packet(
    InputSource * source,
    Output * output,
    TranscodingContext * tctx
){
    int frame_finished;
    int ret;

    ret = (*current_process_handler)(
        tctx->before_decode_video, tctx,
        source, output,
        &tctx->copy_current_packet, NULL, NULL);
    if  (ret < 0)
        die("Fatal error during processing before decode video packet");
    else if (ret == 0) return ret;

    avcodec_decode_video2(source->vctx, tctx->ivframe, &frame_finished, &tctx->curr_packet);
    tctx->ivframe->pts = av_rescale_q_rnd(
        tctx->curr_packet.pts,
        source->video_st->time_base,
        source->vctx->time_base,
        AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX
    );
    if (frame_finished){
        int frame_encoded;
        ret = (*current_process_handler)(
            tctx->after_decode_video, tctx,
            source, output,
            &tctx->copy_current_packet, NULL, NULL);
        if  (ret < 0)
            die("Fatal error during processing after decode video packet");
        else if (ret == 0) return ret;

        tctx->ovframe->pts = tctx->ivframe->pts;

        sws_scale(
            tctx->sws_ctx,
            (uint8_t const * const *)tctx->ivframe->data,
            tctx->ivframe->linesize,
            0,
            source->vctx->height,
            tctx->ovframe->data,
            tctx->ovframe->linesize
        );

        ret = (*current_process_handler)(
            tctx->after_convert_video, tctx,
            source, output,
            &tctx->copy_current_packet, NULL, NULL);
        if  (ret < 0)
            die("Fatal error during processing after convert video frame");
        else if (ret == 0) return ret;

        tctx->ovframe->pts = av_rescale_q_rnd(
                tctx->ivframe->pts,
                source->vctx->time_base,
                output->vctx->time_base,
                AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX
            );
        av_init_packet(&tctx->video_packet);
        tctx->video_packet.data = NULL;
        tctx->video_packet.size = 0;

        ret = (*current_process_handler)(
            tctx->before_encode_video, tctx,
            source, output,
            &tctx->copy_current_packet, &tctx->video_packet, NULL);
        if  (ret < 0)
            die("Fatal error during processing before encode video frame");
        else if (ret == 0) return ret;

        if (avcodec_encode_video2(
            output->vctx,
            &tctx->video_packet,
            tctx->ovframe,
            &frame_encoded
        ) < 0) die("Cannot decode audio packet");
        if (frame_encoded){
            ret = (*current_process_handler)(
                tctx->after_encode_video, tctx,
                source, output,
                &tctx->copy_current_packet, &tctx->video_packet, NULL);
            if  (ret < 0)
                die("Fatal error during processing after encode video frame");
            else if (ret == 0) return ret;

            tctx->video_packet.stream_index = output->video;
            tctx->video_packet.pos = -1;
            tctx->video_packet.pts = av_rescale_q_rnd(
                    tctx->video_packet.pts,
                    output->vctx->time_base,
                    output->video_st->time_base,
                    AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX
                );
            tctx->video_packet.dts = AV_NOPTS_VALUE;
            if (av_interleaved_write_frame(output->ctx, &tctx->video_packet) != 0)
                die("Error while writing video packet");
            av_free_packet(&tctx->curr_packet);
            av_free_packet(&tctx->video_packet);
        }

        return 0;
    }
    return 0;
}

void process_audio_packet(
    InputSource * source,
    Output * output,
    TranscodingContext * tctx
){
    int frame_finished;
    int frame_encoded;
    int ret;

    av_copy_packet(&tctx->copy_current_packet, &tctx->curr_packet);

    ret = (*current_process_handler)(
        tctx->before_decode_audio, tctx,
        source, output,
        &tctx->copy_current_packet, &tctx->curr_packet, NULL);
    if  (ret < 0)
        die("Fatal error during processing before decode audio packet");
    else if (ret == 0) return ret;

    if (avcodec_decode_audio4(
        source->actx,
        tctx->iaframe,
        &frame_finished,
        &tctx->copy_current_packet
    ) < 0) die("Cannot decode audio packet");

    tctx->iaframe->pts = av_rescale_q_rnd(
        tctx->copy_current_packet.pts,
        source->audio_st->time_base,
        source->actx->time_base,
        AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX
    );
    if (frame_finished){
        ret = (*current_process_handler)(
            tctx->after_decode_audio, tctx,
            source, output,
            &tctx->copy_current_packet, &tctx->curr_packet, NULL);
        if (ret < 0)
            die("Fatal error during processing after decode audio packet");
        else if (ret == 0) return ret;

        /*
        INT THIS PLACE THERE IS A NEED TO CHECK BUFFER SIZE OF samples_converted_data
        IF THE IN/OUT FRAME SIZE IS VARIABLE 
        */
        int samples_buffer_size = av_samples_get_buffer_size(
            NULL,
            output->actx->channels,
            tctx->oaframe->nb_samples,
            output->actx->sample_fmt,
            0
        );

        if ( samples_buffer_size != tctx->samples_buffer_size){
            fprintf(stdout,
                "samples_buffer_size = %d\ntctx->samples_buffer_size = %d\n",
            samples_buffer_size, tctx->samples_buffer_size);
            fprintf(stdout, "Reallocating samples_buffer\n" );
             // av_freep(&tctx->samples_buffer_size);
            /*REALLOCATING BUFFER */
            tctx->samples_buffer_size = av_samples_alloc(
                &tctx->samples_converted_data,
                NULL,
                output->actx->channels,
                tctx->oaframe->nb_samples,
                output->actx->sample_fmt,
            0);
            if (tctx->samples_buffer_size < 0) die("Cannot allocate samples");
        };

        int outSamples = swr_convert(
            tctx->swr_ctx,
            NULL,
            0,
            (const uint8_t **)tctx->iaframe->data,
            tctx->iaframe->nb_samples
        );
        if (outSamples < 0 ) die("Cannot convert audio samples");

        for (;;) {
            outSamples = swr_get_out_samples(tctx->swr_ctx, 0);
            if (outSamples < output->actx->frame_size) break;
            outSamples = swr_convert(
                tctx->swr_ctx,
                &tctx->samples_converted_data,
                tctx->oaframe->nb_samples,
                NULL,
                0
            );
            size_t buffer_size = av_samples_get_buffer_size(
                NULL,
                output->actx->channels,
                tctx->oaframe->nb_samples,
                output->actx->sample_fmt,
                0
            );
            if (buffer_size < 0) die("Invalid buffer size");
            if (avcodec_fill_audio_frame(
                tctx->oaframe,
                output->actx->channels,
                output->actx->sample_fmt,
                tctx->samples_converted_data,
                buffer_size,
                0) < 0) die("Could not fill frame");

            ret = (*current_process_handler)(
                tctx->after_convert_audio, tctx,
                source, output,
                &tctx->copy_current_packet, &tctx->curr_packet, NULL);
            if (ret < 0)
                die("Fatal error during processing after convert audio frame");
            else if (ret == 0) return ret;

            //Encoding
            av_init_packet(&tctx->audio_packet);
            tctx->audio_packet.data = NULL;
            tctx->audio_packet.size = 0;

            tctx->oaframe->pts = tctx->iaframe->pts;
            tctx->oaframe->pts = av_rescale_q_rnd(
                tctx->oaframe->pts,
                source->actx->time_base,
                output->actx->time_base,
                AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX
            );
            ret = (*current_process_handler)(
                tctx->before_encode_audio, tctx,
                source, output,
                &tctx->copy_current_packet, &tctx->curr_packet, &tctx->audio_packet);
            if (ret < 0)
                die("Fatal error during processing before encode audio frame");
            else if (ret == 0) return ret;

            if (avcodec_encode_audio2(
                output->actx,
                &tctx->audio_packet,
                tctx->oaframe,
                &frame_encoded) < 0) die("Error encoding audio frame");
            if (frame_encoded){
                ret = (*current_process_handler)(
                    tctx->after_encode_audio, tctx,
                    source, output,
                    &tctx->copy_current_packet, &tctx->curr_packet, &tctx->audio_packet);
                if (ret < 0)
                    die("Fatal error during processing after encode audio packet");
                else if (ret == 0) return ret;

                tctx->audio_packet.stream_index = output->audio;
                tctx->audio_packet.pos = -1;
                tctx->audio_packet.pts = av_rescale_q_rnd(
                    tctx->audio_packet.pts,
                    output->actx->time_base,
                    output->audio_st->time_base,
                    AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX
                );
                tctx->audio_packet.dts = AV_NOPTS_VALUE;

                if (av_interleaved_write_frame(output->ctx, &tctx->audio_packet) !=0 )
                    die("Error while writing audio packet");
                av_free_packet(&tctx->audio_packet);
                av_free_packet(&tctx->copy_current_packet);
                av_free_packet(&tctx->curr_packet);
            }

        }
        //if (convertedData) av_free(&convertedData[0]);
    }
}

int transcode(
    InputSource * source,
    Output * output,
    TranscodingContext * tctx
){
    while(av_read_frame(source->ctx, &tctx->curr_packet) >= 0){
        if (tctx->curr_packet.stream_index == source->video){
            process_video_packet(
                    source,
                    output,
                    tctx
                );
        } else if (tctx->curr_packet.stream_index == source->audio){
            process_audio_packet(
                    source,
                    output,
                    tctx
                );
        }
    }
    return 0;
}
