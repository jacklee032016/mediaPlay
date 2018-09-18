# /bin/bash

# case for wrong URL: JSON reply with FAILED; and can play with correct URL

LOG_FILE="/tmp/cmd.log"
MEDIA_HOME=/media/usb
CASE_NAME=TestCaseTimeout

echo "Test wrong URL and correct URL again....."

echo "beginning..."

echo
echo "playing correct URL..."
sleep 1
apiClient -c play -o media=/g05.mov,index=0 
sleep 2

echo 
echo "playing in-correct URL..."
sleep 1
apiClient -c play -o media=wrongUrl,index=0


sleep 2
echo 
echo "playing correct URL again..."
sleep 1
apiClient -c play -o media=/g05.mov,index=0

