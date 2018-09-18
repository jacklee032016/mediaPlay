#/bin/bash

# This script is for function test of display mode/aspect of window 


apiClient -c play -o media=/g05.mov,index=0 >> /tmp/cmd.log 2>&1
apiClient -c play -o media=/g05.mov,index=1 >> /tmp/cmd.log 2>&1
apiClient -c play -o media=/g05.mov,index=2 >> /tmp/cmd.log 2>&1

echo "Display 2 windows..."
apiClient -c alert -o color=0xFFFF00FF,media="Layout-4" >> /tmp/cmd.log 2>&1
apiClient -c locateWindow -o index=0,left=640,top=0,width=1280,height=720 >> /tmp/cmd.log 2>&1
  
# move hiden windows under main window  
apiClient -c locateWindow -o index=2,left=920,top=600,width=64,height=64 >> /tmp/cmd.log 2>&1
apiClient -c locateWindow -o index=1,left=0,top=0,width=640,height=1080 >> /tmp/cmd.log 2>&1





COUNT=100
i=0

# echo "reset layout..."
# resetLayout

while [ 1 ] # $i -lt $COUNT ]
do
	sleep 5
	n=$((i%4))
	echo "Mode. $i..."
	
	apiClient -c osdFontSize -o index=1,size=80 >> /tmp/cmd.log 2>&1
	apiClient -c osdBackground -o index=1,color=0x00000000 >> /tmp/cmd.log 2>&1
	apiClient -c osdTransparency -o index=1,color=255 >> /tmp/cmd.log 2>&1
	apiClient -c alert -o color=0xFFFF00FF,media="DisplayMode-$n" >> /tmp/cmd.log 2>&1
	apiClient -c aspect -o index=1,mode=$n

	let i+=1
done

# default mode: fill all
# apiClient -c aspect -o index=1,mode=0

# keep ration, adding black border
# apiClient -c aspect -o index=1,mode=1

# keep the ratio and fill
# apiClient -c aspect -o index=1,mode=2

# fill Not keep ratio, sqeezing a little bit
# apiClient -c aspect -o index=1,mode=3

# apiClient -c aspect -o index=1,mode=4
# apiClient -c aspect -o index=1,mode=5

