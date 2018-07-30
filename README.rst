=================
Multi-View Player
=================

Based on ffmpeg and HiSilicon Hi3798C V200 (ARM Cortex A53);

Modules
=======

ffmpeg
-------
* FFMPEG-3.3;
* For mPlayer(ARM), only libavcodec, libavformat and libavutils are neeeded;
* For medis tools on X86 platform, all libraries are needed;
* Build only dependent on itself;

ffmpegTest
----------
* All ffmpeg tools and some examples from ffmpeg;
* Most importances are play and server, used to as testing tool or start points;
* Add 'record' to record from player;


mediaPlayer
-----------
* Multi-View Player support Media server, record, web service, based on plugin infrastructure;
* Every function: record, web service, server, and player all are plugins, which can be enabled/disabled in configuration items;

1. shared library;
~~~~~~~~~~~~~~~~~
2. web server: 
~~~~~~~~~~~~~~
  #. for CGI/static web pages,
  #. for HTTP/HLS(dynamic ts files);
3. record: 
~~~~~~~~~~
* Capture media from player or server;
* Save media into mkv/avi/ts/flv formats;
4. Media Server:
~~~~~~~~~~~~~~~~
* Support protocol: RTSP/HTTP/UDP Multicast(not RTP Multicast);
* Media source from local media file;
* Media from player, eg. behave like a media gateway;

5. mPlayer: multi-view player;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Maximum 6 views(windows) simultaneously;
* Number of windows and position of every window can be changed/hiden dynamically;
* CEC auto negotiation;
* Support independent OSD: alert/logo/subtitle boxs;
* Background, alpha, font color and size, position, size of every OSD box can be changed dynamically;
* Playing video/images(jpeg, png, gif, bmp);
* Playing from local media from USB disk and SD card;
* Playing from network: HTTP/RTSP/RTP/UDP Multicast/MMSH/RTMP
* Playing from playlist which can be defined locally and controlled by internet;
* Controlled by JSON commands from TCP/UDP/Unix sockets;


mediaServers
------------
1 rtmpNginx
~~~~~~~~~~~
* Nginx + RTMP plugin : to test RTMP/HLS protocols;
2 server.2017.07
~~~~~~~~~~~~~~~
* media server of one version;
3 server.2017.09
~~~~~~~~~~~~~~~~
* media server of another version;


resorces
--------
* Some configuration files used onboard;


ecpws
-----

* ECPWS(Embedded Cross-Platform Web Server)
* Web server works in Linux/Windows;


sdk
---
 toolchain from Hisilicon: gcc-4.9.2 + glibc-2.22 

				
