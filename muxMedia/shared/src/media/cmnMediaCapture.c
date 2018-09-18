/*
* encapsulation of ffmpeg routines
*/

/* return time (in second) of packet */
double splayGetPacketTimestamp(AVPacket *pkt, AVFormatContext *ctx, int isPts)
{
	double timeStamp;
	AVRational frameRate;

	AVStream *avStream = ctx->streams[pkt->stream_index];
	if(!avStream)
	{
		MUX_ERROR("No stream found for stream No.%d\n", pkt->stream_index);
		return 0.0;
	}
	
	frameRate = av_guess_frame_rate(ctx, avStream, NULL);

	if(isPts)
		timeStamp = pkt->pts*av_q2d(avStream->time_base);
	else
		timeStamp = pkt->dts*av_q2d(avStream->time_base);

	return timeStamp;//*1.0/av_q2d(frameRate);
}


double get_audio_clock(SimPlay *is)
{
	double pts;
	int hw_buf_size, bytes_per_sec, n;

	pts = is->audio_clock; /* maintained in the audio thread */
	hw_buf_size = is->audio_buf_size - is->audio_buf_index;
	bytes_per_sec = 0;
	n = is->audio_ctx->channels * 2;

	if(is->audio_st)
	{
		bytes_per_sec = is->audio_ctx->sample_rate * n;
	}
	
	if(bytes_per_sec)
	{
		pts -= (double)hw_buf_size / bytes_per_sec;
	}
	return pts;
}


double get_video_clock(MuxMediaCapture *muxCapture)
{
	double delta;
	delta = (av_gettime() - muxCapture->videoCurrentPtsTime) / 1000000.0;
	return muxCapture->videoCurrentPts + delta;
}

double get_external_clock(MuxMediaCapture *muxCapture)
{
	return av_gettime() / 1000000.0;
}

double get_master_clock(MuxMediaCapture *muxCapture)
{
	if(muxCapture->masterClock == MUX_SYNC_MASTER_VIDEO)
	{
		return get_video_clock(muxCapture);
	}
	else if( muxCapture->av_sync_type == MUX_SYNC_MASTER_AUDIO)
	{
		return get_audio_clock(muxCapture);
	}
//	else
	{
		return get_external_clock(muxCapture);
	}
}


