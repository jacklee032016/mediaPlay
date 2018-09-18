# /bin/bash

# case for play image/video with different location


LOG_FILE="/tmp/cmdMixedLocation.log"
MEDIA_HOME=/media/usb
CASE_NAME=TestCaseTimeout

echo "Test Mixed playing image/video and location....."

echo "beginning..."



initAlert(){
	apiClient -c osdFontSize -o index=1,size=80 >> $LOG_FILE 2>&1
	apiClient -c osdBackground -o index=1,color=0x00000000 >> $LOG_FILE 2>&1
	apiClient -c osdTransparency -o index=1,color=255 >> $LOG_FILE 2>&1
#	apiClient -c alert -o color=0xFFFF0000,media="Locate to location No.$1..."  >> $LOG_FILE 2>&1
	return 0
}



location00()
{
		apiClient -c locateWindow -o index=0,left=0,top=0,width=1280,height=1080 >> $LOG_FILE 2>&1
		
		return 0
}

location01()
{
		apiClient -c locateWindow -o index=0,left=640,top=0,width=1280,height=1080 >> $LOG_FILE 2>&1
		
		return 0
}
		

playInOneLocation()
{
	echo "Location No. $1..."
	initAlert
	apiClient -c alert -o color=0xFFFF0000,media="Location No.$1: play g05.mov..."  >> $LOG_FILE 2>&1
	apiClient -c play -o media=/g05.mov >> $LOG_FILE 2>&1
#	checkReturn $?
	sleep 2
	echo "     play image in location No.$1...."
	apiClient -c alert -o color=0xFFFF0000,media="Location No.$1: play index.jpg..."  >> $LOG_FILE 2>&1
	apiClient -c play -o media=/index.jpg >> $LOG_FILE 2>&1
#	checkReturn $?
	
	sleep 2
	echo "     play video in location No.$1...."
	apiClient -c alert -o color=0xFFFF0000,media="Location No.$1: play nature.mp4..."  >> $LOG_FILE 2>&1
	apiClient -c play -o media=/nature.mp4 >> $LOG_FILE 2>&1
#	checkReturn $?
	sleep 3
}

checkReturn()
{
	rc=$1 
	if [[ $rc != 0 ]]; then
		echo "return value is $rc, script exit....."
		exit $rc
	fi
}  
  

while [ 1 ] # $i -lt $COUNT ]
do
	n=$((i%2))
	let i+=1
	
	`location0$n $n`
	playInOneLocation $i
	
done

  