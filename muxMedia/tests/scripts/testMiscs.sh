#/bin/bash

echo "API Miscelleous Testing....."

echo "Service Configuration Info:"
apiClient -c serviceConfig 
echo ""
echo ""
sleep 2

echo "Service Media Feeds Info:"
apiClient -c serviceFeeds 
echo ""
echo ""
sleep 2

echo "Service Configuration Info:"
apiClient -c serviceUrls 
echo ""
echo ""
sleep 2


echo "Service Configuration Info:"
apiClient -c serviceConns 
echo ""
echo ""
sleep 2


echo "Web Info:"
apiClient -c webInfos 
echo ""
echo ""
sleep 2


echo "System Admin Info:"
apiClient -c threads 
echo ""
echo ""

