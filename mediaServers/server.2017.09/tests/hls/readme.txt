
A simple schema for HLS (m38u)


Web Service for HLS media

Request using telnet:
		telnet 192.168.168.111 8083
		GET /video1.m3u8 HTTP/1.1


Following are response:
			HTTP/1.1 206 Partial Content
			Accept-Ranges: bytes
			Content-Length: 176
			Content-Type: application/x-mpegurl
			Connection: close
			
			#EXTM3U
			#EXT-X-MEDIA-SEQUENCE:158302
			#EXT-X-TARGETDURATION:6
			#EXTINF:5,
			0_158197.ts
			#EXTINF:5,
			0_158198.ts
			#EXTINF:5,
			0_158199.ts
			#EXTINF:5,
			0_158200.ts
			#EXTINF:5,
			0_158201.ts


Trying 192.168.168.102...
Connected to 192.168.168.102.
Escape character is '^]'.
GET /video.m3u8 HTTP/1.1
HTTP/1.1 200 OK
Date: Wed, 06 Sep 2017 19:26:57 GMT
Server: ECPWS/0.1.0rc10
Accept-Ranges: bytes
Connection: Keep-Alive
Keep-Alive: timeout=10, max=1010
Content-Length: 133
Last-Modified: Wed, 06 Sep 2017 19:26:49 GMT
Content-Type: application/vnd.apple.mpegurl

#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:11
#EXT-X-MEDIA-SEQUENCE:0
#EXTINF:10.392000,
file000.ts
#EXTINF:9.834000,
file001.ts


HTTP/1.1 206 Partial Content
Date: Wed, 15 Nov 1995 06:25:24 GMT
Last-Modified: Wed, 15 Nov 1995 04:58:08 GMT
Content-Range: bytes 21010-47021/47022
Content-Length: 26012
Content-Type: image/gif

... 26012 bytes of partial image data ...


1. media playlist file:
stream.m3u8
	#EXTM3U
	http://pi.unstabler.pl:8090/stream.ts



2. Stream configuration:
Port 8090
BindAddress 0.0.0.0
MaxHTTPConnections 100

<Stream stream.mkv>
    Feed feed1.ffm
    Format matroska

    AudioCodec libmp3lame

    StartSendOnKey
    AVOptionVideo flags +global_header
    AVOptionAudio flags +global_header
    Preroll 10
</Stream>


<Stream stream.ts>
    Feed feed1.ffm
    Format mpegts
    
    StartSendOnKey
    AVOptionVideo flags +global_header
    AVOptionAudio flags +global_header
    PreRoll 10
</Stream>


3. Player.html
testing whether browsers support HLS
