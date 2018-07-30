# /bin/sh
# 启动Internet WWW Service
# 总是使用一种语音来启动外网的Web服务
#  李志杰 2006.07.25

echo "Start WWW Server"

mkdir -p /tmp/log/boa
touch    /tmp/log/boa/www_error_log
touch    /tmp/log/boa/www_access_log
echo 0 > /tmp/log/boa/www_error_log
echo 0 > /tmp/log/boa/www_access_log

touch    /tmp/log/boa/www_admin_error_log
touch    /tmp/log/boa/www_admin_access_log
echo 0 > /tmp/log/boa/www_admin_error_log
echo 0 > /tmp/log/boa/www_admin_access_log

export LC_ALL=

/usr/bin/www -f /etc/boa/www.conf &
