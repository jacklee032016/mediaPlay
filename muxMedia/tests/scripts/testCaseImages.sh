#/bin/bash

# Integrated testing with layout and playing image/video.
# Dec.20,2017

COUNT=100
LOG_FILE="/tmp/cmdImages.log"

echo "beginning Images Tests..."


addPlaylists()
{
	echo "add playlists and playing..."
	apiClient -c playlistAdd -o media=Pic1,URL=/index.bmp,Duration=2,URL=/index.jpg,Duration=3,URL=/index.png,Duration=2 >> $LOG_FILE 2>&1
	apiClient -c playlistAdd -o media=Pic2,URL=/index.gid,Duration=3,URL=/soldier.jpg,Duration=2,URL=/statuses.png,Duration=3 >> $LOG_FILE 2>&1
	apiClient -c play -o media=Pic1,index=0,repeat=0 >> $LOG_FILE 2>&1
	apiClient -c play -o media=Pic2,index=1,repeat=0  >> $LOG_FILE 2>&1
	apiClient -c play -o media=Pic1,index=2,repeat=0  >> $LOG_FILE 2>&1
	apiClient -c play -o media=Pic2,index=3,repeat=0  >> $LOG_FILE 2>&1
	apiClient -c play -o media=Pic1,index=4,repeat=0  >> $LOG_FILE 2>&1
	apiClient -c play -o media=Pic2,index=5,repeat=0  >> $LOG_FILE 2>&1

	return 0
}

alertMsg()
{
	MSG="Loop No.$1 Layout.$2. "
	apiClient -c osdFontSize -o index=1,size=80 >> $LOG_FILE 2>&1
	apiClient -c osdBackground -o index=1,color=0x00000000 >> $LOG_FILE 2>&1
	apiClient -c osdTransparency -o index=1,color=255 >> $LOG_FILE 2>&1
	apiClient -c alert -o color=0xFFFF0000,media="Loop No.$1, Layout.$2. "  >> $LOG_FILE 2>&1
	return 0
}



layout00 ()
{

MSG="Loop No.$1 Layout.$2. Default"
echo $MSG
#alertMsg '$MSG'

apiClient -c locateWindow -o index=0,left=0,top=0,width=640,height=540 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=1,left=640,top=0,width=640,height=540 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=2,left=1280,top=0,width=640,height=540 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=3,left=0,top=540,width=640,height=540 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=4,left=640,top=540,width=640,height=540 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=5,left=1280,top=540,width=640,height=540 >> $LOG_FILE 2>&1

return 0
return 0

}


layout01 ()
{

MSG="Loop No.$1 Layout.$2. "
echo $MSG
#alertMsg '$MSG'

apiClient -c locateWindow -o index=0,left=0,top=0,width=1280,height=1080 >> $LOG_FILE 2>&1

# move hiden windows under main window  
apiClient -c locateWindow -o index=3,left=120,top=0,width=64,height=64 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=4,left=120,top=120,width=64,height=64 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=5,left=120,top=240,width=64,height=64 >> $LOG_FILE 2>&1

# move window2 before window1 or don't move window2, because window keeps same location

apiClient -c locateWindow -o index=2,left=1280,top=540,width=640,height=540 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=1,left=1280,top=0,width=640,height=540 >> $LOG_FILE 2>&1

return 0

}


layout02 ()
{

MSG="Loop No.$1 Layout.$2. "
echo $MSG
#alertMsg "$MSG"

apiClient -c locateWindow -o index=0,left=640,top=0,width=1280,height=1080 >> $LOG_FILE 2>&1

# move hiden windows under main window  
apiClient -c locateWindow -o index=3,left=720,top=0,width=64,height=64 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=4,left=720,top=120,width=64,height=64 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=5,left=720,top=240,width=64,height=64 >> $LOG_FILE 2>&1

apiClient -c locateWindow -o index=1,left=0,top=0,width=640,height=540 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=2,left=0,top=540,width=640,height=540 >> $LOG_FILE 2>&1

return 0

}


layout03 ()
{

MSG="Loop No.$1 Layout.$2. "
echo $MSG
#alertMsg "$MSG"

apiClient -c locateWindow -o index=0,left=0,top=540,width=1920,height=540 >> $LOG_FILE 2>&1

# move hiden windows under main window  
apiClient -c locateWindow -o index=4,left=1120,top=640,width=64,height=64 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=5,left=1240,top=640,width=64,height=64 >> $LOG_FILE 2>&1
  
apiClient -c locateWindow -o index=1,left=0,top=0,width=640,height=540 >> $LOG_FILE 2>&1
# window3 must be moved before window2
apiClient -c locateWindow -o index=3,left=1280,top=0,width=640,height=540 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=2,left=640,top=0,width=640,height=540 >> $LOG_FILE 2>&1
return 0

}


layout04 ()
{

MSG="Loop No.$1 Layout.$2. "
echo $MSG
#alertMsg "$MSG"

apiClient -c locateWindow -o index=0,left=320,top=0,width=1280,height=1080 >> $LOG_FILE 2>&1
  
# move hiden windows under main window  
apiClient -c locateWindow -o index=3,left=720,top=700,width=64,height=64 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=4,left=720,top=820,width=64,height=64 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=5,left=720,top=940,width=64,height=64 >> $LOG_FILE 2>&1

apiClient -c locateWindow -o index=1,left=0,top=0,width=320,height=1080 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=2,left=1600,top=0,width=320,height=1080 >> $LOG_FILE 2>&1
return 0

}


layout05 ()
{

MSG="Loop No.$1 Layout.$2. "
echo $MSG
#alertMsg $MSG 
	
apiClient -c locateWindow -o index=0,left=0,top=810,width=1920,height=270 >> $LOG_FILE 2>&1

# move hiden windows under main window  
apiClient -c locateWindow -o index=3,left=400,top=820,width=64,height=64 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=4,left=520,top=820,width=64,height=64 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=5,left=640,top=820,width=64,height=64 >> $LOG_FILE 2>&1

apiClient -c locateWindow -o index=1,left=0,top=0,width=960,height=810 >> $LOG_FILE 2>&1
apiClient -c locateWindow -o index=2,left=960,top=0,width=960,height=810 >> $LOG_FILE 2>&1

return 0

}


echo "Integrated Tests beginning..."

i=0

alertMsg  "Initilize testing...."
addPlaylists

echo "start testing..."

while [ 1 ] # $i -lt $COUNT ]
do

	n=$((i%6))
	echo "Loop No.$i Layout No.$n..."
	
	cmd="layout0$n $i $n"
	$cmd

	alertMsg $i $n 

	sleep 2
	let i+=1
	
done

