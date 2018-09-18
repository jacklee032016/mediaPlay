#/bin/bash

# test different layouts (6 layouts) playing image or video .

COUNT=100
LOG_FILE="/tmp/cmd.log"

echo "beginning..."

apiClient -c alert -o color=0xFFFF0000,media="Set Banner is transparent..."  >> /tmp/cmd.log 2>&1

apiClient -c osdFontSize -o index=1,size=80 >> $LOG_FILE 2>&1
# no background color
apiClient -c osdBackground -o index=1,color=0x00000000 >> $LOG_FILE 2>&1
# no transprency for text
apiClient -c osdTransparency -o index=1,color=255 >> $LOG_FILE 2>&1

initPlays()
{
# select test images or video with all the layouts
	MEDIA_FILE=/index.jpg
#	MEDIA_FILE=/g05.mov
	
	apiClient -c play -o index=0,media=$MEDIA_FILE  >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=1,media=$MEDIA_FILE  >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=2,media=$MEDIA_FILE  >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=3,media=$MEDIA_FILE  >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=4,media=$MEDIA_FILE  >> /tmp/cmd.log 2>&1
	apiClient -c play -o index=5,media=$MEDIA_FILE  >> /tmp/cmd.log 2>&1

	return 0
}

layout00() {
	apiClient -c alert -o color=0xFFFF0000,media="Default Layout"  >> /tmp/cmd.log 2>&1
	
  apiClient -c locateWindow -o index=0,left=0,top=0,width=640,height=540 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=1,left=640,top=0,width=640,height=540 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=2,left=1280,top=0,width=640,height=540 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=3,left=0,top=540,width=640,height=540 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=4,left=640,top=540,width=640,height=540 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=5,left=1280,top=540,width=640,height=540 >> /tmp/cmd.log 2>&1

  return 0
}


layout01(){
	apiClient -c alert -o color=0xFF00FF00,media="Layout-1" >> /tmp/cmd.log 2>&1
	
  apiClient -c locateWindow -o index=0,left=0,top=0,width=1280,height=1080 >> /tmp/cmd.log 2>&1

# move hiden windows under main window  
  apiClient -c locateWindow -o index=3,left=120,top=0,width=64,height=64 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=4,left=120,top=120,width=64,height=64 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=5,left=120,top=240,width=64,height=64 >> /tmp/cmd.log 2>&1

# move window2 before window1 or don't move window2, because window keeps same location
  apiClient -c locateWindow -o index=2,left=1280,top=540,width=640,height=540 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=1,left=1280,top=0,width=640,height=540 >> /tmp/cmd.log 2>&1
  

	return 0;
}


layout02(){
	apiClient -c alert -o color=0xFF0000FF,media="Layout-2" >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=0,left=640,top=0,width=1280,height=1080 >> /tmp/cmd.log 2>&1

# move hiden windows under main window  
  apiClient -c locateWindow -o index=3,left=720,top=0,width=64,height=64 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=4,left=720,top=120,width=64,height=64 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=5,left=720,top=240,width=64,height=64 >> /tmp/cmd.log 2>&1

  apiClient -c locateWindow -o index=1,left=0,top=0,width=640,height=540 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=2,left=0,top=540,width=640,height=540 >> /tmp/cmd.log 2>&1

	return 0;
}


layout03(){
	apiClient -c alert -o color=0xFFFFFF00,media="Layout-3" >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=0,left=0,top=540,width=1920,height=540 >> /tmp/cmd.log 2>&1

# move hiden windows under main window  
  apiClient -c locateWindow -o index=4,left=1120,top=640,width=64,height=64 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=5,left=1240,top=640,width=64,height=64 >> /tmp/cmd.log 2>&1
  
  apiClient -c locateWindow -o index=1,left=0,top=0,width=640,height=540 >> /tmp/cmd.log 2>&1
# window3 must be moved before window2
  apiClient -c locateWindow -o index=3,left=1280,top=0,width=640,height=540 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=2,left=640,top=0,width=640,height=540 >> /tmp/cmd.log 2>&1

	return 0;
}


layout04(){
	apiClient -c alert -o color=0xFFFF00FF,media="Layout-4" >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=0,left=320,top=0,width=1280,height=1080 >> /tmp/cmd.log 2>&1
  
# move hiden windows under main window  
  apiClient -c locateWindow -o index=3,left=920,top=700,width=64,height=64 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=4,left=920,top=820,width=64,height=64 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=5,left=920,top=940,width=64,height=64 >> /tmp/cmd.log 2>&1

  apiClient -c locateWindow -o index=1,left=0,top=0,width=320,height=1080 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=2,left=1600,top=0,width=320,height=1080 >> /tmp/cmd.log 2>&1

	return 0;
}


layout05(){
	apiClient -c alert -o color=0xFF00FFFF,media="Layout-5" >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=0,left=0,top=810,width=1920,height=270 >> /tmp/cmd.log 2>&1
  
# move hiden windows under main window  
  apiClient -c locateWindow -o index=3,left=400,top=820,width=64,height=64 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=4,left=520,top=820,width=64,height=64 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=5,left=640,top=820,width=64,height=64 >> /tmp/cmd.log 2>&1

  apiClient -c locateWindow -o index=1,left=0,top=0,width=960,height=810 >> /tmp/cmd.log 2>&1
  apiClient -c locateWindow -o index=2,left=960,top=0,width=960,height=810 >> /tmp/cmd.log 2>&1

	return 0;
}



echo "testing..."

i=0

# echo "reset layout..."
# resetLayout

initPlays

while [ 1 ] # $i -lt $COUNT ]
do
	sleep 2
	n=$((i%6))
	echo "Layout No. $i..."
	`layout0$n`
	let i+=1
done

