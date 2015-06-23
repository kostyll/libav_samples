/**
 * FFmpeg sample player with audio support and syncing.
 * Compile with:
 * gcc player2.c -o player -lavutil -lavformat -lavcodec -lswscale -lz -lbz2 `/opt/SDL/bin/sdl-config --cflags --libs`
 * Related to: http://habrahabr.ru/blogs/video/138426/
 * Author: Alexey Zhuchkov (zerodivisi0n)
 * License: GPL
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <SDL.h>

#define SDL_AUDIO_BUFFER_SIZE 1024
#define AUDIO_BUF_SIZE ((AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2)
#define MAX_AUDIO_BUF_SIZE (AVCODEC_MAX_AUDIO_FRAME_SIZE * 4)
/* no AV sync correction is done if below the AV sync threshold */
#define AV_SYNC_THRESHOLD 0.01
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

#define FF_REFRESH_EVENT (SDL_USEREVENT + 1)

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    int eof;
    SDL_mutex* mutex;
    SDL_cond* cond;
} PacketQueue;

typedef struct RingBuffer {
    uint8_t* data;
    int size;
    int max_size;
    int rindex;  // Read position
    int windex;  // Write position
    char eof;  // EOF flag
    char lastop;  // last operation flag: 0 - read, 1 - write
    SDL_mutex* mutex;
    SDL_cond* rcond;
    SDL_cond* wcond;
} RingBuffer;

typedef struct VideoPicture {
    SDL_Surface* screen;
    SDL_Overlay* bmp;
    double pts;
    int ready;
    SDL_mutex* mutex;
    SDL_cond* cond;
} VideoPicture;

typedef struct MainContext {
    AVFormatContext *format_context;
    int quit;
    
    // Streams
    AVStream* video_stream;
    AVStream* audio_stream;
    
    // Queues
    PacketQueue videoq;
    PacketQueue audioq;
    
    struct SwsContext* sws_context;
    
    VideoPicture pict;
    RingBuffer audio_buf;
    
    double video_clock;
    double audio_clock;
    
    double frame_timer;
    double frame_last_pts;
    double frame_last_delay;
    
    // Threads
    SDL_Thread* demux_tid;
    SDL_Thread* video_decode_tid;
    SDL_Thread* audio_decode_tid;
} MainContext;

static void usage(const char* progname) {
    assert(progname != NULL);
    
    fprintf(stderr, "Usage %s movie\n", progname);
}

static void packet_queue_init(PacketQueue* q) {
    assert(q != NULL);
    
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

static void packet_queue_deinit(PacketQueue* q) {
    assert(q != NULL);
    assert(q->mutex != NULL);
    assert(q->cond != NULL);
    
    SDL_DestroyMutex(q->mutex);
    SDL_DestroyCond(q->cond);
}

static int packet_queue_put(PacketQueue* q, AVPacket* pkt) {
    assert(q != NULL);
    assert(pkt != NULL);
    
    // Duplicate current packet
    if (av_dup_packet(pkt) < 0) {
        return -1;
    }
    
    AVPacketList* pkt_list = av_malloc(sizeof(AVPacketList));
    if (pkt_list == NULL) {
        return -1;
    }
    
    pkt_list->pkt = *pkt;
    pkt_list->next = NULL;
    
    SDL_LockMutex(q->mutex);
    
    if (!q->eof) {
        if (q->last_pkt == NULL) {
            // It's a first packet in queue
            q->first_pkt = pkt_list;
        } else {
            // Append to the end of queue
            q->last_pkt->next = pkt_list;
        }
        q->last_pkt = pkt_list;
        q->nb_packets++;
        q->size += pkt->size;
    }
    SDL_CondSignal(q->cond);
    
    SDL_UnlockMutex(q->mutex);
    return 0;
}

static int packet_queue_get(PacketQueue* q, AVPacket* pkt, int block) {
    assert(q != NULL);
    assert(pkt != NULL);
    
    SDL_LockMutex(q->mutex);
    
    AVPacketList* pkt_list = q->first_pkt;
    if (!block && (pkt_list == NULL)) {
        SDL_UnlockMutex(q->mutex);
        return -1;
    } else {
        while ((pkt_list == NULL) && !q->eof) {
            // Wait for packets
            SDL_CondWait(q->cond, q->mutex);
            pkt_list = q->first_pkt;
        }
    }
    
    if ((pkt_list == NULL) && q->eof) {
        SDL_UnlockMutex(q->mutex);
        return -1;
    }
    
    q->first_pkt = pkt_list->next;
    if (q->first_pkt == NULL) {
        // No more packets
        q->last_pkt = NULL;
    }
    *pkt = pkt_list->pkt;
    q->nb_packets--;    
    q->size -= pkt->size;
    av_free(pkt_list);
    
    SDL_UnlockMutex(q->mutex);
    return 0;
}

static void packet_queue_eof(PacketQueue* q) {
    assert(q != NULL);
    
    SDL_LockMutex(q->mutex);
    q->eof = 1;
    SDL_CondBroadcast(q->cond);
    SDL_UnlockMutex(q->mutex);
}

int ring_buffer_init(RingBuffer* rb, int initial_size, int max_size) {
    assert(rb != NULL);
    assert((max_size <= 0) || (max_size >= initial_size));
    
    memset(rb, 0, sizeof(RingBuffer));
    
    rb->data = av_malloc(initial_size);
    if (rb->data == NULL) {
        return -1;
    }
    rb->size = initial_size;
    rb->max_size = max_size;
    rb->mutex = SDL_CreateMutex();
    rb->rcond = SDL_CreateCond();
    rb->wcond = SDL_CreateCond();
    return 0;
}

void ring_buffer_deinit(RingBuffer* rb) {
    assert(rb != NULL);
    assert(rb->mutex != NULL);
    assert(rb->rcond != NULL);
    assert(rb->wcond != NULL);
    
    SDL_DestroyMutex(rb->mutex);
    SDL_DestroyCond(rb->rcond);
    SDL_DestroyCond(rb->wcond);
}

// Returns number of bytes written
int ring_buffer_write(RingBuffer* rb, void* buffer, int len, int block) {
    assert(rb != NULL);
    assert(rb != NULL);
    
    if (len == 0) {
        return 0;
    }
    
    SDL_LockMutex(rb->mutex);
    
    if (rb->eof) {
        // Buffer ended
        SDL_UnlockMutex(rb->mutex);
        return -1;
    }
    
    uint8_t* buffer_ptr = buffer;
    while (len > 0) {
        int step;
        if ((rb->rindex < rb->windex) ||  // write forward
            ((rb->rindex == rb->windex) && (rb->lastop == 0))) {  // buffer must be free
            step = rb->size - rb->windex;
        } else if (rb->rindex > rb->windex) {
            step = rb->rindex - rb->windex;
        } else if (block) {
            SDL_CondWait(rb->rcond, rb->mutex);
            if (rb->eof) {
                break;
            }
            continue;
        } else {
            break;
        }
        
        if (len < step) {
            step = len;
        }
        memcpy(rb->data + rb->windex, buffer_ptr, step);
        
        rb->windex += step;
        assert(rb->windex <= rb->size);
        if (rb->windex == rb->size) {
            rb->windex = 0;
        }
        rb->lastop = 1;
        SDL_CondSignal(rb->wcond);
        
        buffer_ptr += step;
        len -= step;
    }
    
    SDL_UnlockMutex(rb->mutex);
    return buffer_ptr - (uint8_t*)buffer;
}

// Returns number of bytes read
int ring_buffer_read(RingBuffer* rb, void* buffer, int len, int block) {
    assert(rb != NULL);
    assert(buffer != NULL);
    
    if (len == 0) {
        return 0;
    }
    
    SDL_LockMutex(rb->mutex);
    
    uint8_t* buffer_ptr = buffer;
    while (len > 0) {
        int step;
        if (rb->rindex < rb->windex) {  // write forward
            step = rb->windex - rb->rindex;
        } else if ((rb->rindex > rb->windex) ||  // read forward
                   ((rb->rindex == rb->windex) && (rb->lastop == 1))) {  // buffer must be full
            step = rb->size - rb->rindex;
        } else if (block && !rb->eof) {
            SDL_CondWait(rb->wcond, rb->mutex);
            continue;
        } else {
            break;
        }
        
        if (len < step) {
            step = len;
        }
        memcpy(buffer_ptr, rb->data + rb->rindex, step);
        
        rb->rindex += step;
        assert(rb->rindex <= rb->size);
        if (rb->rindex == rb->size) {
            rb->rindex = 0;
        }
        rb->lastop = 0;
        SDL_CondSignal(rb->rcond);
        
        buffer_ptr += step;
        len -= step;
    }
    
    int count = buffer_ptr - (uint8_t*)buffer;
    if ((count == 0) && rb->eof) {
        count = -1;
    }
    
    SDL_UnlockMutex(rb->mutex);
    return count;
}

int ring_buffer_size(RingBuffer* rb) {
    assert(rb != NULL);
    
    int size;
    SDL_LockMutex(rb->mutex);
    if (rb->rindex < rb->windex) {
        size = rb->windex - rb->rindex;
    } else {
        size = rb->size - rb->rindex + rb->windex;
    }
    SDL_UnlockMutex(rb->mutex);
    return size;
}

void ring_buffer_eof(RingBuffer* rb) {
    assert(rb != NULL);
    
    SDL_LockMutex(rb->mutex);
    rb->eof = 1;
    SDL_CondBroadcast(rb->wcond);
    SDL_UnlockMutex(rb->mutex);
}

static AVStream* open_stream(AVFormatContext* format_context, int type) {
    assert(format_context != NULL);
    
    int index;
    AVStream* stream = NULL;
    // Find stream index
    for (index = 0; index < format_context->nb_streams; ++index) {
        if (format_context->streams[index]->codec->codec_type == type) {
            stream = format_context->streams[index];
            break;
        }
    }
    if (stream == NULL) {
        // Stream index not found
        return NULL;
    }
    
    AVCodecContext* codec_context = stream->codec;
    
    // Find suitable codec
    AVCodec* codec = avcodec_find_decoder(codec_context->codec_id);
    if (codec == NULL) {
        // Codec not found
        return NULL;
    }
    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        // Failed to open codec
        return NULL;
    }
    
    return stream;
}

static void close_stream(AVStream* stream) {
    assert(stream != NULL);
    
    if (stream->codec != NULL) {
        avcodec_close(stream->codec);
    }
}

static void audio_callback(void* userdata, uint8_t* stream, int len) {
    assert(userdata != NULL);
    
    MainContext* main_context = (MainContext*)userdata;
    ring_buffer_read(&main_context->audio_buf, stream, len, 1);
}

static int configure_audio(MainContext* main_context) {
    assert(main_context != NULL);
    
    AVCodecContext* codec_context = main_context->audio_stream->codec;
    
    SDL_AudioSpec wanted_spec, spec;
    // Set audio settings from codec info
    wanted_spec.freq = codec_context->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = codec_context->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = main_context;
    
    if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
      fprintf(stderr, "SDL: %s\n", SDL_GetError());
      return -1;
    }

    SDL_PauseAudio(0);
    
    return 0;
}

static int open_file(MainContext* main_context, const char* filename) {
    assert(main_context != NULL);
    assert(filename != NULL);
    
    int err;

    // Open video file
    AVFormatContext* format_context = NULL;
    err = avformat_open_input(&format_context, filename, NULL, NULL);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to open input file\n");
        return -1;
    }
    main_context->format_context = format_context;

    // Retrieve stream information
    err = avformat_find_stream_info(format_context, NULL);
    if (err < 0) {
        fprintf(stderr, "ffmpeg: Unable to find stream info\n");
        return -1;
    }
    
    // Dump information about file onto standard error
    av_dump_format(format_context, 0, filename, 0);
    
    // Open video and audio streams
    main_context->video_stream = open_stream(format_context, AVMEDIA_TYPE_VIDEO);
    if (main_context->video_stream == NULL) {
        fprintf(stderr, "ffmpeg: Could not open video stream\n");
        return -1;
    }
    main_context->audio_stream = open_stream(format_context, AVMEDIA_TYPE_AUDIO);
    if (main_context->audio_stream == NULL) {
        fprintf(stderr, "ffmpeg: Could not open audio stream\n");
        return -1;
    }
    
    // Create conversion context
    AVCodecContext* video_codec_context = main_context->video_stream->codec;
    main_context->sws_context = sws_getCachedContext(NULL,
                                                     video_codec_context->width, video_codec_context->height,
                                                     video_codec_context->pix_fmt,
                                                     video_codec_context->width, video_codec_context->height,
                                                     PIX_FMT_YUV420P, SWS_BICUBIC,
                                                     NULL, NULL, NULL);
    
    // Prepare SDL video output
    main_context->pict.screen = SDL_SetVideoMode(video_codec_context->width, video_codec_context->height, 0, 0);
    if (main_context->pict.screen == NULL) {
        fprintf(stderr, "Couldn't set video mode\n");
        return -1;
    }
    
    main_context->pict.bmp = SDL_CreateYUVOverlay(video_codec_context->width, video_codec_context->height,
                                                  SDL_YV12_OVERLAY, main_context->pict.screen);
    if (main_context->pict.bmp == NULL) {
        fprintf(stderr, "Couldn't create YUV overlay\n");
        return -1;
    }
    
    // Set initial timer value
    main_context->frame_timer = (double)av_gettime() / 1000000.0;
    
    return 0;
}

// Main demux loop
static int demux_thread(void* arg) {
    assert(arg != NULL);
    
    MainContext* main_context = (MainContext*)arg;
    
    int video_stream_index = main_context->video_stream->index;
    int audio_stream_index = main_context->audio_stream->index;
    AVPacket packet;
    while (av_read_frame(main_context->format_context, &packet) >= 0) {
        if (main_context->quit) {
            av_free_packet(&packet);
            break;
        }
        if (packet.stream_index == video_stream_index) {
            // Video packet
            packet_queue_put(&main_context->videoq, &packet);
        } else if (packet.stream_index == audio_stream_index) {
            // Audio packet
            packet_queue_put(&main_context->audioq, &packet);
        } else {
            av_free_packet(&packet);
        }
    }
    packet_queue_eof(&main_context->videoq);
    packet_queue_eof(&main_context->audioq);
    
    return 0;
}

double synchronize_video(MainContext* main_context, AVFrame *src_frame, double pts) {
    assert(main_context != NULL);
    assert(src_frame != NULL);
    
    AVCodecContext* video_codec_context = main_context->video_stream->codec;
    if(pts != 0) {
        /* if we have pts, set video clock to it */
        main_context->video_clock = pts;
    } else {
        /* if we aren't given a pts, set it to the clock */
        pts = main_context->video_clock;
    }
    /* update the video clock */
    double frame_delay = av_q2d(video_codec_context->time_base);
    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    main_context->video_clock += frame_delay;
    
    return pts;
}

static int video_decode_thread(void* arg) {
    assert(arg != NULL);
    
    MainContext* main_context = (MainContext*)arg;
    AVCodecContext* video_codec_context = main_context->video_stream->codec;
    
    AVFrame frame;
    while (!main_context->quit) {
        avcodec_get_frame_defaults(&frame);
        
        // Get packet from queue
        AVPacket pkt;
        if (packet_queue_get(&main_context->videoq, &pkt, 1) < 0) {
            // eof queue
            break;
        }
        
        int got_frame;
        int len = avcodec_decode_video2(video_codec_context, &frame, &got_frame, &pkt);
        if (len < 0) {
            av_free_packet(&pkt);
            fprintf(stderr, "Failed to decode video frame\n");
            break;
        }
        
        if (got_frame) {
            // Consider sync
            double pts = frame.pkt_dts;
            if (pts == AV_NOPTS_VALUE) {
                pts = frame.pkt_pts;
            }
            if (pts == AV_NOPTS_VALUE) {
                pts = 0;
            }
            pts *= av_q2d(main_context->video_stream->time_base);
            pts = synchronize_video(main_context, &frame, pts);
            
            // Wait until picture is released
            SDL_LockMutex(main_context->pict.mutex);
            while (main_context->pict.ready && !main_context->quit) {
                SDL_CondWait(main_context->pict.cond, main_context->pict.mutex);
            }
            SDL_UnlockMutex(main_context->pict.mutex);
            
            if (main_context->quit) {
                break;
            }
            
            SDL_Overlay* bmp = main_context->pict.bmp;
            main_context->pict.pts = pts;
            
            // Convert frame to YV12 pixel format for display in SDL overlay
            SDL_LockYUVOverlay(bmp);
            
            AVPicture pict;
            pict.data[0] = bmp->pixels[0];
            pict.data[1] = bmp->pixels[2];  // it's because YV12
            pict.data[2] = bmp->pixels[1];
            
            pict.linesize[0] = bmp->pitches[0];
            pict.linesize[1] = bmp->pitches[2];
            pict.linesize[2] = bmp->pitches[1];
            
            sws_scale(main_context->sws_context,
                      frame.data, frame.linesize,
                      0, video_codec_context->height,
                      pict.data, pict.linesize);
            
            SDL_UnlockYUVOverlay(bmp);
            
            SDL_LockMutex(main_context->pict.mutex);
            main_context->pict.ready = 1;
            SDL_CondSignal(main_context->pict.cond);
            SDL_UnlockMutex(main_context->pict.mutex);
        }
    }
    
    return 0;
}

static int audio_decode_thread(void *arg) {
    assert(arg != NULL);
    
    MainContext* main_context = (MainContext*)arg;
    AVCodecContext* audio_codec_context = main_context->audio_stream->codec;
    
    AVFrame frame;
    while (!main_context->quit) {
        avcodec_get_frame_defaults(&frame);
        
        // Get packet from queue
        AVPacket pkt;
        if (packet_queue_get(&main_context->audioq, &pkt, 1) < 0) {
            // eof queue
            break;
        }
        
        // The audio packet can contain several frames
        int got_frame;
        int len = avcodec_decode_audio4(audio_codec_context, &frame, &got_frame, &pkt);
        if (len < 0) {
            av_free_packet(&pkt);
            fprintf(stderr, "Failed to decode audio frame\n");
            break;
        }
        
        if (got_frame) {
            // Store frame
            // Get decoded buffer size
            int data_size = av_samples_get_buffer_size(NULL, audio_codec_context->channels,
                                                        frame.nb_samples,
                                                        audio_codec_context->sample_fmt, 1);
            
            // Obtain audio clock
            if (pkt.pts != AV_NOPTS_VALUE) {
                main_context->audio_clock = av_q2d(main_context->audio_stream->time_base) * pkt.pts;
            } else {
                /* if no pts, then compute it */
                main_context->audio_clock += (double)data_size /
                                                        (audio_codec_context->channels *
                                                        audio_codec_context->sample_rate *
                                                        av_get_bytes_per_sample(audio_codec_context->sample_fmt));
            }
            ring_buffer_write(&main_context->audio_buf, frame.data[0], data_size, 1);
        }
        av_free_packet(&pkt);
    }
    ring_buffer_eof(&main_context->audio_buf);
    return 0;
}

static uint32_t sdl_refresh_timer(uint32_t interval, void *opaque) {
  SDL_Event event;
  event.type = FF_REFRESH_EVENT;
  event.user.data1 = opaque;
  SDL_PushEvent(&event);
  return 0;
}

static void schedule_refresh(MainContext* main_context, int delay) {
    assert(main_context != 0);
    assert(delay > 0.0);
    
    SDL_AddTimer(delay, sdl_refresh_timer, main_context);
}

static double get_audio_clock(MainContext* main_context) {
    assert(main_context != NULL);
    
    AVCodecContext* audio_codec_context = main_context->audio_stream->codec;
    double pts = main_context->audio_clock;
    
    int bytes_per_sec = audio_codec_context->sample_rate *
                            audio_codec_context->channels *
                            av_get_bytes_per_sample(audio_codec_context->sample_fmt);
    int buffer_size = ring_buffer_size(&main_context->audio_buf);
    
    if (bytes_per_sec != 0) {
        pts -= (double)buffer_size / bytes_per_sec;
    }
    
    return pts;
}

static void video_display(MainContext* main_context) {
    AVCodecContext* video_codec_context = main_context->video_stream->codec;
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = video_codec_context->width;
    rect.h = video_codec_context->height;
    SDL_DisplayYUVOverlay(main_context->pict.bmp, &rect);
}

static double compute_delay(MainContext* main_context) {
    double delay = main_context->pict.pts - main_context->frame_last_pts;
    if (delay <= 0.0 || delay >= 1.0) {
        // Delay incorrect - use previous one
        delay = main_context->frame_last_delay;
    }
    // Save for next time
    main_context->frame_last_pts = main_context->pict.pts;
    main_context->frame_last_delay = delay;

    // Update delay to sync to audio
    double ref_clock = get_audio_clock(main_context);
    double diff = main_context->pict.pts - ref_clock;
    double sync_threshold = FFMAX(AV_SYNC_THRESHOLD, delay);
    if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
        if (diff <= -sync_threshold) {
            delay = 0;
        } else if (diff >= sync_threshold) {
            delay = 2 * delay;
        }
    }

    main_context->frame_timer += delay;
    
    double actual_delay = main_context->frame_timer - (av_gettime() / 1000000.0);
    if(actual_delay < 0.010) {
        /* Really it should skip the picture instead */
        actual_delay = 0.010;
    }
    return actual_delay;
}

static void video_refresh_timer(MainContext* main_context) {
    SDL_LockMutex(main_context->pict.mutex);
    while ((main_context->pict.ready == 0) && !main_context->quit) {
        SDL_CondWait(main_context->pict.cond, main_context->pict.mutex);
    }
    SDL_UnlockMutex(main_context->pict.mutex);
    
    if (main_context->quit) {
        return;
    }
    
    // Sync video to audio
    double delay = compute_delay(main_context);
    schedule_refresh(main_context, (int)(delay * 1000 + 0.5));
    
    // Show the picture
    video_display(main_context);
    
    SDL_LockMutex(main_context->pict.mutex);
    main_context->pict.ready = 0;
    SDL_CondSignal(main_context->pict.cond);
    SDL_UnlockMutex(main_context->pict.mutex);
}

static MainContext* allocate_context() {
    MainContext* main_context = av_mallocz(sizeof(MainContext));
    if (main_context == NULL) {
        // memory allocation failed
        return NULL;
    }
    
    packet_queue_init(&main_context->videoq);
    packet_queue_init(&main_context->audioq);
    ring_buffer_init(&main_context->audio_buf, AUDIO_BUF_SIZE, MAX_AUDIO_BUF_SIZE);
    
    main_context->pict.mutex = SDL_CreateMutex();
    main_context->pict.cond = SDL_CreateCond();
    
    return main_context;
}

static void free_context(MainContext* main_context) {
    assert(main_context != NULL);
    
    // Close streams
    close_stream(main_context->video_stream);
    close_stream(main_context->audio_stream);
    
    packet_queue_deinit(&main_context->videoq);
    packet_queue_deinit(&main_context->audioq);
    ring_buffer_deinit(&main_context->audio_buf);
    
    // Free SDL resources
    SDL_FreeSurface(main_context->pict.screen);
    SDL_FreeYUVOverlay(main_context->pict.bmp);
    
    // Close file
    if (main_context->format_context != NULL)  {
        avformat_close_input(&main_context->format_context);
    }
}

static void quit(MainContext* main_context) {
    main_context->quit = 1;
    SDL_CondBroadcast(main_context->pict.cond);
}

static void event_loop(MainContext* main_context) {
    assert(main_context != NULL);
    
    SDL_Event event;
    while (!main_context->quit) {
        SDL_WaitEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
                quit(main_context);
                break;
            case FF_REFRESH_EVENT:
                video_refresh_timer(main_context);
                break;
        }
    }
}

typedef int (SDLCALL *ThreadProc)(void *);

static SDL_Thread* thread_start(ThreadProc fn, void* userdata, const char* name) {
    assert(fn != NULL);
    
    SDL_Thread* thread = SDL_CreateThread(fn, userdata);
    if (thread == NULL) {
        fprintf(stderr, "SDL: Failed to run '%s' thread - %s\n", name, SDL_GetError());
    }
    return thread;
}

static int thread_wait(SDL_Thread* tid, const char* name) {
    int status;
    SDL_WaitThread(tid, &status);
    printf("Thread '%s' finished with status %d\n", name, status);
    return status;
}

int main(int argc, char* argv[]) {
    int err;
    // Check arguments
    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }
    
    // Register all available file formats and codecs
    av_register_all();
    
    // Init SDL
    err = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    if (err < 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return -1;
    }
    
    MainContext* main_context = allocate_context();

    const char* filename = argv[1];
    err = open_file(main_context, filename);
    if (err < 0) {
        return -1;
    }
    
    configure_audio(main_context);
    
    // Run required threads
    main_context->demux_tid = thread_start(demux_thread, main_context, "demux");
    main_context->video_decode_tid = thread_start(video_decode_thread, main_context, "video_decode");
    main_context->audio_decode_tid = thread_start(audio_decode_thread, main_context, "audio_decode");
    
    // Start video refresh
    schedule_refresh(main_context, 40);

    // Start event loop
    event_loop(main_context);
    
    // Wait until all the threads will be finished
    thread_wait(main_context->demux_tid, "demux");
    thread_wait(main_context->video_decode_tid, "video_decode");
    thread_wait(main_context->audio_decode_tid, "audio_decode");
    
    // Free resources
    free_context(main_context);
    SDL_Quit();
    
    return 0;
}