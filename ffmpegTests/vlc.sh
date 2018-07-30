# /bin/bash
# Jack Lee. Start VLC stream server in Linux

# MEDIA_FILE=../m/iMPACT.Wrestling.2015.11.05.1080p-30.WEBRip.h264-spamTV.mkv
# hevc is not support by RTP
#MEDIA_FILE=../usb/Samsung-UHD-Iceland.ts
# MEDIA_FILE=../usb/John.Wick.Chapter.2.2017.1080p.WEB-DL.mp4
MEDIA_FILE=../m/American.mkv

# OUT_HTTP=standard{access=http,mux=mkv,dst=:8088}
OUT_HTTP=http{mux=ffmpeg{mux=flv},dst=:8088/httpTests}

OUT_RTSP=rtp{sdp=rtsp://:8554/rtspTests}
OUT_MMSH=std{access=mmsh,mux=asfh,dst=0.0.0.0:8090}

# OUT_RTP=rtp{dst=0.0.0.0,port=5004,mux=ts}
# OUT_RTPM=rtp{dst=239.0.0.1,port=5005,mux=ts}
OUT_RTP=rtp{dst=0.0.0.0,port=5004,mux=ts,sap,name=rtpTests}
OUT_RTPM=rtp{dst=239.0.0.1,port=5004,mux=ts,sap,name=rtpTests}

OUT_UDP=udp{dst=0.0.0.0:1234}
OUT_UDPM=udp{dst=239.0.0.2:1234}


VLC_CMD=cvlc

ADDRESS=192.168.168.102

#if [ "$1" == "" ]; then
if [ -z "$1"  ]; then
	echo "   Usage: 'vlc.sh PROTOCOL(HTTP|RTSP|MMSH|RTP|RTPM|UDP|UDPM|ALL) [MEDIA FILE]'"
	exit 1
fi
if [ "$2" != "" ]; then
		MEDIA_FILE=$2
fi


echo "vlc begin with $MEDIA_FILE on $1..."

case $1 in
        HTTP|http)
            echo "HTTP"
            $VLC_CMD -vvv $MEDIA_FILE --sout "#$OUT_HTTP"
            ;;
        RTSP|rtsp)
            echo "RTSP"
            $VLC_CMD -vvv $MEDIA_FILE --sout "#$OUT_RTSP"
            ;;
        WMSP|wmsp|MMSH|mmsh)
            echo "MMSH"
            $VLC_CMD -vvv $MEDIA_FILE --sout "#$OUT_MMSH"
            ;;
        RTP|rtp)
            echo "RTP"
            $VLC_CMD -vvv $MEDIA_FILE --sout "#$OUT_RTP"
            ;;
        UDP|udp)
            echo "UDP"
            $VLC_CMD -vvv $MEDIA_FILE --sout "#$OUT_UDP"
            ;;
        RTPM|rtpm)
            echo "RTP Multicasr"
            $VLC_CMD -vvv $MEDIA_FILE --sout "#$OUT_RTPM"
            ;;
        UDPM|UDPM)
            echo "UDP Multicast"
            $VLC_CMD -vvv $MEDIA_FILE --sout "#$OUT_UDPM"
            ;;
        ALL|all)
            echo "All Protocols(HTTP|RTSP|MMSH|RTP|UDP)"
            echo " '$OUT_HTTP': $OUT_RTSP: $OUT_MMSH:$OUT_RTP:$OUT_RTPM:$OUT_UDP:$OUT_UDPM"
#            exit 1
            $VLC_CMD -vvv $MEDIA_FILE --sout "#duplicate{dst=$OUT_HTTP,dst=$OUT_RTSP,dst=$OUT_MMSH,dst=$OUT_RTP,dst=$OUT_RTPM,dst=$OUT_UDP,dst=$OUT_UDPM}"
            ;;
        *)
            echo -e "Error: $0 invalid option '$1'\nTry '$0 --help' for more information.\n" >&2
            exit 1
            ;;
esac
