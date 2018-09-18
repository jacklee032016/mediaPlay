# /bin/bash

# test HiPlayer response to stop command, after EOF event response is OK

LOG_FILE="/tmp/cmd.log"
MEDIA_HOME=/media/usb
CASE_NAME=TestCaseTimeout

echo "Test timeout/stop command when only play video....."

echo "beginning..."

apiClient -c playlistAdd -o media=$CASE_NAME,URL=$MEDIA_HOME/g05.mov,Duration=1 >> $LOG_FILE 2>&1

apiClient -c play -o media=$CASE_NAME,index=0 >> $LOG_FILE 2>&1
apiClient -c play -o media=$CASE_NAME,index=1 >> $LOG_FILE 2>&1
apiClient -c play -o media=$CASE_NAME,index=2 >> $LOG_FILE 2>&1
apiClient -c play -o media=$CASE_NAME,index=3 >> $LOG_FILE 2>&1
apiClient -c play -o media=$CASE_NAME,index=4 >> $LOG_FILE 2>&1
apiClient -c play -o media=$CASE_NAME,index=5 >> $LOG_FILE 2>&1

