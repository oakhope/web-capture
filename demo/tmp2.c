AVFrame *readAVFrame(AVCodecContext *pCodecCtx, AVFormatContext *pFormatCtx, AVFrame *pFrameRGB, int videoStream, int ms) {
    struct SwsContext *sws_ctx = NULL;

    AVPacket packet;
    AVFrame *pFrame = NULL;

    pFrame = av_frame_alloc();

    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

    int timeStamp = ((double)ms / (double)1000) * pFormatCtx->streams[videoStream]->time_base.den / pFormatCtx->streams[videoStream]->time_base.num;

    int ret = av_seek_frame(pFormatCtx, videoStream, timeStamp, AVSEEK_FLAG_BACKWARD);

    if (ret < 0) {
        fprintf(stderr, "av_seek_frame failed\n");
        return NULL;
    }

    int64_t target_pts = av_rescale_q(ms, (AVRational){1, 1000}, pFormatCtx->streams[videoStream]->time_base);

    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        if (packet.stream_index == videoStream) {
            if (avcodec_send_packet(pCodecCtx, &packet) != 0) {
                fprintf(stderr, "avcodec_send_packet failed\n");
                av_packet_unref(&packet);
                continue;
            }

            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                if (pFrame->pts >= target_pts) {
                    sws_scale(sws_ctx, (uint8_t const *const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

                    sws_freeContext(sws_ctx);
                    av_frame_free(&pFrame);
                    av_packet_unref(&packet);

                    return pFrameRGB;
                }
            }
        }
    }

    sws_freeContext(sws_ctx);
    av_frame_free(&pFrame);
    av_packet_unref(&packet);

    return NULL;
}