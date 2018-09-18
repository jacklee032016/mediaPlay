#/bin/bash

if [ -z "$1" ]
  then
    echo "upload all files to ARM board, defined as '$0 IP'"
    exit 0
fi

echo "upload libraries to $1/lib..."
scp ./Linux.bin.arm/lib/* root@$1:/lib/
echo ""

echo "upload binaries to $1..."
scp ./Linux.bin.arm/sbin/* root@$1:/sbin/
scp ./Linux.bin.arm/usr/bin/* root@$1:/usr/bin/
scp ./Linux.bin.arm/var/admin/cgi-bin/*  root@$1:/var/admin/cgi-bin/
echo ""

echo "upload configuration files to $1/etc/mLab..."
scp ./releases/etc/mLab/*  root@$1:/etc/mLab/

