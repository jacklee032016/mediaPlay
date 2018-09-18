#/bin/bash

if [ -z "$1" ]
  then
    echo "Downloaing file should be defined as '$0 MEDIA_FILE'"
    exit 0
fi

echo "download, get file info, play, stop and remove the downloaded file"

apiClient -c download -o media=$1,server=192.168.168.102,user=anonymous,password=lzj320@123.com,path=/pub

sleep 3
apiClient -c file -o media=/media/usb/$1

apiClient -c play -o media=/media/usb/$1
sleep 5

apiClient -c stop

apiClient -c fileDelete -o file=/media/usb/$1

