

UDP multicast support in ffmpeg:

		ffmpeg -i iMPACT.mkv -c copy -f mpegts udp://239.0.0.1:12345


HLS:
./ffmpeg -v 9 -loglevel 99 -re -i sourcefile.avi -c copy -c:v libx264 -b:v 128k -vpre ipod320 -flags -global_header -map 0 -f segment -segment_time 4 \
-segment_list test.m3u8 -segment_format mpegts stream%05d.ts


ffmpeg -re -i "%WMSAPP_HOME%/content/sample.mp4"  -vcodec libx264  -vb 150000 -g 60 -vprofile baseline -level 2.1 -acodec aac -ab 64000 -ar 48000 -ac 2 -vbsf h264_mp4toannexb -strict experimental -f mpegts udp://127.0.0.1:10000

 -hls_wrap 10 -start_number 1 <pathToFolderYouWantTo>/<streamName>.m3u8

Design
		One or multiple media sources of H264 or HEVC video + AAC audio;
		One thread is used to read media and send data to FIFO;
		
		One Thread monitors service port of RTSP/HTTP to create new data connection;
		FSM of HTTP/RTSP
		
		One thread read data from one FIFO, and send data to data connections;
		


July,25, 2017
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
		
