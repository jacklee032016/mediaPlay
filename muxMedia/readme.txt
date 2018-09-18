					README for Mux Player/Server (RX769)
									May, 2017 Jack Lee

ChangeLog
	Sep.30, 2017
			Integrate 3 programs (Player/Server/WebService) together, and reconstruct all from every parts;
	
	Sep.27, 2017
			Port to ARM board, and test basic functions on ARM board;

	Sep.27, 2017
			Port to ARM board, and test basic functions on ARM board;
			
	Sep.26, 2017
			Implement and check the accuracy of statistics of server and its every connection;

	Sep.25, 2017
			Implement and debug status page of servicing connections, streams and media feeds;
			
	Sep.20,2017
			All protocols and some media files have been tested. 
			
	Sep.,6th, 2017
			All protocols can run in new framework;


July,17th, 2017
		Build and run on X86 platform, no dependent on cmdutils, just dependent on ffmpeg.
		Access stat.html in port of 9090.

July,4th, 2017
		For ARM platform, it is only linked to static libraries: ffmpeg and libShared.
		Modify links options in Makefile in MuxServer and MuxWin (arm/tests/muxPlay):
				muxServer size is more than 4MB when linked with static libraries; is about 160KB when linked with shared libraries;


Requirements for running on board:
		/root/higo_gb2312.ubf
				this file exists in SAMPLES/res/font
		/etc/muxMedia.conf
		
		/vid.mkv
				media file after player start
						
After start, if these are not configed, it will be broken



How to compile:
	Build for ARM:
		make ARCH=arm 
	Build for X86
		make ARCH=X86

	Build as independent programs to test: muxWeb not support it
		make PLUGINS=NO ARCH=X86		

		
Test programs mainly for X86 platform.

record vid.h264 test.mkv 

play .test.mkv -loglevel 99 2>&1 | tee tests.log


					