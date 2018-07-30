
# this file has ass subtitle which is not supported by RTP
# File "/media/sf_mux/m/4k/HEVC.10bit.60fps.mkv"

# this file has AC3 audio which is not supported by RTP
File "/media/sf_mux/m/4k/bbb.mp4"


Feed:
		Live feed, only come from ffmpeg or ffserver
		
Stream:
		Media streams come from Feed or local media file;
		Same feed used in multiple output streams, with different codec format;
		Transcode from input stream into output format;
		
		Format:
				transcode for output from feed or local media???
				different types of stream is determined by format field:
					mpeg: MPEG-1 muxplexied video/audio
					mpegvideo: MPEG-1 only video
					mp2: mpeg-2 audio
					
					These formats are determined by ffmpeg library or just be server;
					
		
		Every stream can be access from HTTP:http://host:port/filename
		Filename of the HTTP stream with following extension will be redirected to:
				asx
				asf
				rpm/ram
				rtsp
				sdp


		Stream Types:
				Redirect: like the redirect;
				stat: web page
				live: all streams except stat, are live stream


mmsh protocol:
		configure stream as asf (extension is asf, format is asf), after redirect it, the client (VLC displays it as mmsh stream)
		
