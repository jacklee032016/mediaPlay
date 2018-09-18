#/bin/bash

# test the so-called bugs from web page: sometimes window can't be update after moving to a new position.
# Jan.22nd,2018.

COUNT=100
LOG_FILE="/tmp/cmd.log"

echo "beginning..."

rm -rf /tmp/cmd.log

apiClient -c alert -o color=0xFFFF0000,media="Set Banner is transparent..."  >> /tmp/cmd.log 2>&1

apiClient -c osdFontSize -o index=1,size=80 >> $LOG_FILE 2>&1
# no background color
apiClient -c osdBackground -o index=1,color=0x00000000 >> $LOG_FILE 2>&1
# no transprency for text
apiClient -c osdTransparency -o index=1,color=255 >> $LOG_FILE 2>&1

initPlays()
{
	return 0
}


layout00(){
	apiClient -c locateWindow -o index=0,left=0,top=0,width=1280,height=1080  >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=1,left=1280,top=0,width=640,height=540  >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=2,left=1280,top=540,width=640,height=540  >> /tmp/cmd.log 2>&1

	return 0;
}

play00(){
	apiClient -c playlistAdd -o media="LAYOUT1-MAIN",URL="/media/usb/GoPro HERO6  This Is the Moment in 4K.mp4",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=0,media=LAYOUT1-MAIN >> /tmp/cmd.log 2>&1

	apiClient -c playlistAdd -o media=LAYOUT1-PIP1,URL="/media/usb/GoPro HERO4  The Adventure of Life in 4K.mp4",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=1,media=LAYOUT1-PIP1 >> /tmp/cmd.log 2>&1

	apiClient -c playlistAdd -o media=LAYOUT1-PIP2,URL="/media/usb/Fashion Show Milan 4K Demo.ts",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=2,media=LAYOUT1-PIP2 >> /tmp/cmd.log 2>&1

	return 0;
}

layout01(){
	apiClient -c locateWindow -o index=0,left=640,top=0,width=1280,height=1080  >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=1,left=0,top=0,width=640,height=540  >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=2,left=0,top=540,width=640,height=540  >> /tmp/cmd.log 2>&1

	return 0;
}

play01(){
	apiClient -c playlistAdd -o media=LAYOUT2-MAIN,URL="/media/usb/Fashion Show Milan 4K Demo.ts",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=0,media=LAYOUT2-MAIN >> /tmp/cmd.log 2>&1

	apiClient -c playlistAdd -o media=LAYOUT2-PIP1,URL="/media/usb/Soccer_Barcelona_Atletico_Madrid.ts",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=1,media=LAYOUT2-PIP1 >> /tmp/cmd.log 2>&1

	apiClient -c playlistAdd -o media=LAYOUT2-PIP2,URL="/media/usb/Soccer_Barcelona_Atletico_Madrid.ts",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=2,media=LAYOUT2-PIP2 >> /tmp/cmd.log 2>&1

	return 0;
}


layout06(){
	apiClient -c locateWindow -o index=0,left=0,top=540,width=1920,height=540  >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=2,left=640,top=0,width=640,height=540  >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=3,left=1280,top=0,width=640,height=540  >> /tmp/cmd.log 2>&1 
	apiClient -c locateWindow -o index=1,left=0,top=0,width=640,height=540  >> /tmp/cmd.log 2>&1

	return 0;
}


play06(){
	apiClient -c playlistAdd -o media=LAYOUT3-MAIN,URL="/media/usb/GoPro HERO6  This Is the Moment in 4K.mp4",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=0,media=LAYOUT3-MAIN >> /tmp/cmd.log 2>&1
	
	apiClient -c playlistAdd -o media=LAYOUT3-PIP1,URL="/media/usb/GoPro HERO4  The Adventure of Life in 4K.mp4",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=1,media=LAYOUT3-PIP1 >> /tmp/cmd.log 2>&1
	
	apiClient -c playlistAdd -o media=LAYOUT3-PIP2,URL="/media/usb/GoPro HERO4  The Adventure of Life in 4K.mp4",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=2,media=LAYOUT3-PIP2 >> /tmp/cmd.log 2>&1

	return 0;
}


layout02(){
	apiClient -c locateWindow -o index=0,left=320,top=0,width=1280,height=1080  >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=1,left=0,top=0,width=320,height=1080  >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=2,left=1600,top=0,width=320,height=1080  >> /tmp/cmd.log 2>&1

	return 0;
}

play02(){
	apiClient -c playlistAdd -o media=LAYOUT4-MAIN,URL="/media/usb/Soccer_Barcelona_Atletico_Madrid.ts",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=0,media=LAYOUT4-MAIN >> /tmp/cmd.log 2>&1
	
	apiClient -c playlistAdd -o media=LAYOUT4-PIP1,URL="/media/usb/coke.jpg",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=1,media=LAYOUT4-PIP1 >> /tmp/cmd.log 2>&1
	
	apiClient -c playlistAdd -o media=LAYOUT4-PIP2,URL="/media/usb/coke2.jpg",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=2,media=LAYOUT4-PIP2 >> /tmp/cmd.log 2>&1

	return 0;
}


layout03(){
	apiClient -c locateWindow -o index=0,left=0,top=810,width=1920,height=270  >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=1,left=0,top=0,width=960,height=810  >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=2,left=960,top=0,width=960,height=810  >> /tmp/cmd.log 2>&1

	return 0;
}


play03(){
	apiClient -c playlistAdd -o media=LAYOUT5-MAIN,URL="/media/usb/Popcorn.jpg",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=0,media=LAYOUT5-MAIN >> /tmp/cmd.log 2>&1
	
	apiClient -c playlistAdd -o media=LAYOUT5-PIP1,URL="/media/usb/Interstellar-TLR-F2-5.1-4K-HDTN.mp4",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=1,media=LAYOUT5-PIP1 >> /tmp/cmd.log 2>&1
	
	apiClient -c playlistAdd -o media=LAYOUT5-PIP2,URL="/media/usb/Elysium_trailer_1-4K-HDTN(4ksamples.com).mp4",Duration=200 >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=2,media=LAYOUT5-PIP2 >> /tmp/cmd.log 2>&1

	return 0;
}

layout05() {
	apiClient -c locateWindow -o index=0,left=0,top=0,width=1920,height=1080  >> /tmp/cmd.log 2>&1
  return 0
}



cronResetLayout(){                                                                                                                                                                                          
	apiClient -c locateWindow -o index=1,left=960,top=815,width=64,height=64 >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=2,left=960,top=900,width=64,height=64 >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=3,left=1040,top=815,width=64,height=64 >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=4,left=1040,top=900,width=64,height=64 >> /tmp/cmd.log 2>&1
	apiClient -c locateWindow -o index=5,left=1110,top=1000,width=64,height=64 >> /tmp/cmd.log 2>&1    
	return 0;
}

echo "testing..."

i=0

# echo "reset layout..."
# resetLayout

initPlays

while [ 1 ] # $i -lt $COUNT ]
do
	sleep 5
	n=$((i%4))
	echo "Layout No. $i..."
	cronResetLayout
	`layout0$n`
	`play0$n`
	let i+=1
done

