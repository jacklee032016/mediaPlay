				README for Media Player from ffmpeg
										Oct.27th, 2017 		Jack Lee


Synchronization
		Happens when output audio and video, eg.SDL audio callback, and SDL event handle for video output;
				playVideoFresh() and sdlAudioCallback()
				
		Sync modes:
				Sync to audio: 
						audio output as usual, eg. update audio clock with PTS from audio frame/packet;
						video frame should be replicated or dropped based on the driff between audio clock and video clock;
						video clock is updated based on operation of replication or drop;
						works fine in VM;
				Sync to video: 
						decoder has statisitcs of fault DTS/PTS;


File Lists:

		playMain.c 						: entry pointer, create read thread;
		playThreadRead.c			: initialize the player, and read media data from source;
		playStream.c					: open/close streams in source when player is initialzing;
		playThreadMedia.c			: video/audio/subtitle 3 decoding threads, read from packet queue, and send to frame queue;

		playVideoRefresh.c		: refresh video(subtitle), called in event handler of SDL;
		playFilter.c					:	libavfilter for player;
		
		playSdlEventLoop.c		: main thread, eg. event loop of SDL;
		playSdlDisplay.c			: SDL render and texture management;
		playSdlAudio.c				: Initialize SDL audio, and audio callback (get audio frame from frame queue), and update audio clock;
		
		playQueue.c						: Queue management of Packet Queue and Frame Queue;
		playUtils.c						: Decoder (general decode interface for video/audio/subtitle), Clock;
		playOpts.c						: options for player;
										