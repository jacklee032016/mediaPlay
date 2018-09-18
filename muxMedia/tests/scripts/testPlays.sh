#/bin/bash

echo "playlist and play testing....."

if [ -z "$1" ]
  then
    echo "defined as '$0 USB|SD directory'"
    exit 0
fi


echo "Create videos only playlist..."
apiClient -c playlistAdd -o media=VideosShow,URL=$1/We.Bare.mkv,Duration=10,URL=$1/g05.mov,Duration=0,URL=$1/t15.mov,Duration=5
echo ""
echo ""
sleep 2

echo "Create images only playlist..."
apiClient -c playlistAdd -o media=ImagesShow,URL=$1/building.jpg,Duration=10,URL=$1/soldier.jpg,Duration=5,URL=$1/statuses.jpg,Duration=5
echo ""
echo ""
sleep 2


echo "Create mixed playlist..."
apiClient -c playlistAdd -o media=Mixed,URL=$1/index.jpg,Duration=10,URL=$1/We.Bare.mkv,Duration=10,URL=$1/index.png,Duration=5,URL=$1/g05.mov,Duration=0,URL=$1/statuses.jpg,Duration=5,URL=$1/t15.mov,Duration=5
echo ""
echo ""
sleep 2


