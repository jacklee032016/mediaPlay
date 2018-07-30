
										README for RTMP Usage and Tests
																								August 22nd, 2017         Jack Lee

########################################
Debugging 
########################################
	test RTMP with TX server: 192.168.168.102/live
	
	Log of RTMP server:
			/usr/local/nginx/logs/logs/access.log
			/usr/local/nginx/logs/logs/error.log
			
			you check the connecting or PUBLISH/PLAY request and error info 



########################################
Build 
########################################

install pcre and openssl: dnf install pcre-devel|openssl-devel
configure --add-module=../rtmp-nginx --with-debug
make; make install
NGINX_HOME=/usr/local/nginx/* 
Add following into NGINX_HOME/conf/nginx.conf:
rtmp {
        server {
                listen 1935;
                chunk_size 4096;
 
                application live {
                        live on;
                        record off;
                }
        }
}

error_log logs/error.log debug;


########################################
Protocol Tests
########################################

ffmpeg -re -i iMPACT.mkv -f flv rtmp://192.168.168.102:1935/jackSvr

ffmpeg -re -i iMPACT.mkv -c copy -f flv rtmp://192.168.168.102:1935/jackSvr

play rtmp://192.168.168.102/jackSvr


ffmpeg -re -i iMPACT.mkv -c copy -f flv rtmp://192.168.168.102:1935/hls

./Linux.bin.X86/usr/bin/play rtmp://192.168.168.102/hls



 ffmpeg -f video4linux2 -v verbose -r 30 -s 640x480 -i /dev/video0 -c:v libx264 -c:a libmp3lame -b:v 400k -b:a 64k -flags -global_header -map 0 -f segment -segment_format mpegts -segment_list_type m3u8 -segment_list /var/www/live/mystream.m3u8 -segment_list_flags +live -segment_wrap 6 -segment_time 10 /var/www/live/mystream-%08d.ts
 


########################################
NGINX operations:
########################################

Test the configuration file
/usr/local/nginx/sbin/nginx -t

Start nginx in the background

/usr/local/nginx/sbin/nginx

Start nginx in the foreground

/usr/local/nginx/sbin/nginx -g 'daemon off;'

Reload the config on the go

/usr/local/nginx/sbin/nginx -t && nginx -s reload

Kill nginx

/usr/local/nginx/sbin/nginx -s stop
