
vlc --reset-config --reset-plugins-cache

cvlc  -vvv ../m/tests/4K-29-H264.mp4 \
		--sout '#standard{access=http,mux=mkv,dst=192.168.168.102:8080}'


cvlc  -vvv ../m/tests/4K-29-H264.mp4 \
		--sout '#rtp{sdp=rtsp://:8554/}'

# note: the last '/' must be added
play rtsp://192.168.168.102:8554/

cvlc  -vvv ../m/American.mkv \
		--sout '#std{access=mmsh,mux=asfh,dst=0.0.0.0:8090}'

play mmsh://192.168.168.102:8090/


cvlc  -vvv ../m/American.mkv \
		--sout '#rtp{dst=0.0.0.0,port=5004,mux=ts}'

play rtp://192.168.168.102:5004/

		
cvlc  -vvv ../m/American.mkv \
		--sout '#rtp{dst=239.0.0.1,port=5005,mux=ts}'

cvlc  -vvv ../m/60.Minutes.mkv \
		--sout '#rtp{dst=239.0.0.1,port=5005,mux=ts}'

play udp://239.0.0.1:5005/

cvlc  -vvv ../m/American.mkv \
		--sout '#udp{dst=0.0.0.0:1234}'

play udp://192.168.168.102:1234/


cvlc  -vvv ../m/American.Housewife.S01E18.720p.HDTV.x264-AVS.mkv \
		--sout '#transcode{vcodec=h264,acodec=mpga,ab=128,channels=2,samplerate=44100}:udp{dst=239.0.0.2:1234}'

cvlc  -vvv ../m/American.Housewife.S01E18.720p.HDTV.x264-AVS.mkv \
		--sout '#transcode{vcodec=none,acodec=mp3,ab=128,channels=2,samplerate=44100}:udp{dst=239.0.0.2:1234}'

#transcode{vcodec=h264,acodec=mpga,ab=128,channels=2,samplerate=44100}:

=#transcode{vcodec=none,acodec=mp3,ab=128,channels=2,samplerate=44100}

:sout=#duplicate{
	dst=http{mux=ffmpeg{mux=flv},dst=:8080/httpTests},
	dst=std{access=mmsh,mux=asfh,dst=0.0.0.0:8080},
	dst=rtp{sdp=rtsp://:8554/rtpsTests}
	} :sout-keep	

:sout=#duplicate{
	dst=http{mux=ffmpeg{mux=flv},dst=:8080/httpTests},
	dst=std{access=mmsh,mux=asfh,dst=0.0.0.0:8080},
	dst=rtp{sdp=rtsp://:8554/rtpsTests},
	dst=rtp{dst=239.0.0.3,port=5004,mux=ts,sap,name=rtpTests},
	dst=udp{dst=239.0.0.1:1234}
} :sout-keep		