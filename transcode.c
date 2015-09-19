#include "general.h"

int process_video_packet(
    InputSource * source,
    Output * output,
    TranscodingContext * tctx
){
    int frame_finished;

    avcodec_decode_video2(source->vctx, tctx->ivframe, &frame_finished, &tctx->curr_packet);
    tctx->ivframe->pts = av_rescale_q_rnd(
        tctx->curr_packet.pts,
        source->video_st->time_base,
        source->vctx->time_base,
        AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX
    );
    if (frame_finished){
        int frame_encoded;
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

        tctx->ovframe->pts = av_rescale_q_rnd(
                tctx->ivframe->pts,
                source->vctx->time_base,
                output->vctx->time_base,
                AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX
            );
        av_init_packet(&tctx->video_packet);
        tctx->video_packet.data = NULL;
        tctx->video_packet.size = 0;

        if (avcodec_encode_video2(
            output->vctx,
            &tctx->video_packet,
            tctx->ovframe,
            &frame_encoded
        ) < 0) die("Cannot decode audio packet");
        if (frame_encoded){
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

int process_audio_packet(
    InputSource * source,
    Output * output,
    TranscodingContext * tctx
){
    int frame_finished;
    int frame_encoded;
    int dst_nb_samples;

    av_copy_packet(&tctx->copy_current_packet, &tctx->curr_packet);
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
        uint8_t ** convertedData = NULL;
        if (av_samples_alloc(
            &convertedData,
            NULL,
            source->actx->channels,
            tctx->oaframe->nb_samples,
            output->actx->sample_fmt,
            0) < 0) die("Cannot allocate samples");
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
                &convertedData,
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
                convertedData,
                buffer_size,
                0) < 0) die("Could not fill frame");

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
            if (avcodec_encode_audio2(
                output->actx,
                &tctx->audio_packet,
                tctx->oaframe,
                &frame_encoded) < 0) die("Error encoding audio frame");
            if (frame_encoded){
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
        if (convertedData) av_free(&convertedData[0]);
    }
}

int transcode(
	InputSource * source,
	Output * output,
	TranscodingContext * tctx
){
	int ret;
	while(av_read_frame(source->ctx, &tctx->curr_packet) >= 0){
        if (tctx->curr_packet.stream_index == source->video){
            ret = process_video_packet(
                    source,
                    output,
                    tctx
                );
        } else if (tctx->curr_packet.stream_index == source->audio){
            ret = process_audio_packet(
                    source,
                    output,
                    tctx
                );
        }
    }
    return 0;
}