#/bin/bash

# test image/video playing with API and alert, without location change.
# Dec.20,2017


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

alertMsg()
{
	
	apiClient -c osdFontSize -o index=1,size=80 >> /tmp/cmd.log 2>&1
	apiClient -c osdBackground -o index=1,color=0x00000000 >> /tmp/cmd.log 2>&1
	apiClient -c osdTransparency -o index=1,color=255 >> /tmp/cmd.log 2>&1

	apiClient -c alert -o color=0xFFFF0000,media="Loop-$1"  >> /tmp/cmd.log 2>&1
	
	return 0
}



randomPlay()
{
	MEDIA=/g06.mov
	num=$((($RANDOM)%5))
	
	if [ $num -eq 0 ]
	then
		MEDIA=/soldier.jpg
	else if [ $num -eq 1 ]	
		then
			MEDIA=/index.jpg
		else if [ $num -eq 2 ]	
			then
				MEDIA=/t10.mov
			else if [ $num -eq 3 ]	
				then
					MEDIA=/building.jpg
				else 
					MEDIA=/g05.mov
				fi	
			fi
		fi
	fi						

#	echo "play $MEDIA on window $1 when $num"
	apiClient -c play -o media=$MEDIA,index=$1,repeat=0 >> /tmp/cmdImages.log 2>&1
	
	return 0
}


echo "Image Tests beginning..."

i=0


echo "start testing..."

layout00

while [ 1 ] # $i -lt $COUNT ]
do
	n=$((i%6))
	echo "Loop No.$i Layout No.$n..."
	
	randomPlay 0
	randomPlay 1
	randomPlay 2
	randomPlay 3
	randomPlay 4
	randomPlay 5


 	alertMsg $i

	sleep 2
	let i+=1
	
done

