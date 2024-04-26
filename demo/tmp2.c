AVFrame *readLastFrame(AVCodecContext *pCodecCtx, AVFormatContext *pFormatCtx, AVFrame *pFrameRGB, int videoStream) {
    struct SwsContext *sws_ctx = NULL;

    AVPacket packet;
    AVFrame *pFrame = NULL;
    AVFrame *lastFrame = NULL;

    pFrame = av_frame_alloc();

    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

    int ret = av_seek_frame(pFormatCtx, videoStream, INT64_MAX, AVSEEK_FLAG_BACKWARD);

    if (ret < 0) {
        fprintf(stderr, "av_seek_frame failed\n");
        return NULL;
    }

    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        if (packet.stream_index == videoStream) {
            if (avcodec_send_packet(pCodecCtx, &packet) != 0) {
                fprintf(stderr, "avcodec_send_packet failed\n");
                av_packet_unref(&packet);
                continue;
            }

            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                if (lastFrame != NULL) {
                    av_frame_free(&lastFrame);
                }
                lastFrame = av_frame_clone(pFrame);
            }
        }
    }

    sws_scale(sws_ctx, (uint8_t const *const *)lastFrame->data, lastFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

    sws_freeContext(sws_ctx);
    av_frame_free(&pFrame);
    av_packet_unref(&packet);

    return pFrameRGB;
}