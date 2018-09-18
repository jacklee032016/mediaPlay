# /bin/bash

# case for STOP command: send STOP command and restart playing before the playing ends

LOG_FILE="/tmp/cmdStop.log"
MEDIA_HOME=/media/usb
CASE_NAME=TestCaseTimeout

echo "Test wrong URL and correct URL again....."

echo "beginning..."


initAlert(){
	apiClient -c osdFontSize -o index=1,size=80 >> $LOG_FILE 2>&1
	apiClient -c osdBackground -o index=1,color=0x00000000 >> $LOG_FILE 2>&1
	apiClient -c osdTransparency -o index=1,color=255 >> $LOG_FILE 2>&1
	apiClient -c alert -o color=0xFFFF0000,media="Start/Stop No.$1..."  >> /tmp/cmd.log 2>&1
	return 0
}


start(){
	apiClient -c play -o media="/g05.mov",index=0 >> $LOG_FILE 2>&1
	apiClient -c play -o media="/g05.mov",index=1 >> $LOG_FILE 2>&1
	apiClient -c play -o media="/g05.mov",index=2 >> $LOG_FILE 2>&1
	apiClient -c play -o media="/g05.mov",index=3 >> $LOG_FILE 2>&1
	apiClient -c play -o media="/g05.mov",index=4 >> $LOG_FILE 2>&1
	apiClient -c play -o media="/g05.mov",index=5 >> $LOG_FILE 2>&1

	return 0;
}

stop(){
	apiClient -c stop -o index=0 >> $LOG_FILE 2>&1
	apiClient -c stop -o index=1 >> $LOG_FILE 2>&1
	apiClient -c stop -o index=2 >> $LOG_FILE 2>&1
	apiClient -c stop -o index=3 >> $LOG_FILE 2>&1
	apiClient -c stop -o index=4 >> $LOG_FILE 2>&1
	apiClient -c stop -o index=5 >> $LOG_FILE 2>&1

	return 0;
}


echo "testing..."

i=0

# echo "reset STOP command..."
# resetLayout

while [ 1 ] # $i -lt $COUNT ]
do
	sleep 2
	n=$((i%6))
	let i+=1
#	apiClient -c alert -o color=0xFFFF0000,media="Start/Stop No.$i..."  >> /tmp/cmd.log 2>&1
	initAlert $i
	echo "Stop/Start No. $i..."
	start 
done
