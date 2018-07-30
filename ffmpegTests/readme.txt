					README for tests of FFMPEG 3.3
									May, 2017 Jack Lee

Test programs mainly for X86 platform.


record vid.h264 test.mkv 

play .test.mkv -loglevel 99 2>&1 | tee tests.log

play -s 1916*956 -pix_fmt yuv420p -f rawvideo yourfile
					