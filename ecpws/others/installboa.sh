#!/bin/sh
# 将安装完成的libpam软件目录打包、发布
# 李志杰 2006.07.07
#

SRCDIR=$2
DATDIR=`echo $1 | sed 's/\/\//\//g'`
PKGDIR="$DATDIR/../pkg"

	echo ""
	echo ""
	echo "Install BOA WebServer ....."
	echo "Build Directris in $PKGDIR"
	if [ -d $PKGDIR ]; then 
		echo "Remove the Exist $PKGDIR"
		rm -rf $PKGDIR
	fi
	
	mkdir -p $PKGDIR/tmp/log/boa
	touch $PKGDIR/tmp/log/boa/www_access_log
	touch $PKGDIR/tmp/log/boa/www_error_log
	touch $PKGDIR/tmp/log/boa/admin_access_log
	touch $PKGDIR/tmp/log/boa/admin_error_log
	
	echo "Copy Configuration Files into $PKGDIR"
	cp -r $SRCDIR/* $PKGDIR

#	echo "Copy Test Web Pages into $PKGDIR"
#	cp -r $SRCDIR/var $PKGDIR

	CVS_FILES=`find $PKGDIR -name \CVS `
	for f in $CVS_FILES; do
		echo "          Remove $f"
		rm -rf $f
	done

	echo "Copy EXEs into $PKGDIR"
	cp -r $DATDIR/* $PKGDIR
	cd $PKGDIR/usr/bin
	ln -s boa www
	cd $PKGDIR
	tar czf $ASSIST_RELEASES_NAME *
	mv $ASSIST_RELEASES_NAME $ROOT_DIR

cat << EOF
======================================================================
BOA(Web Server) installed 
     _________________________________________________________
     *****  $ASSIST_RELEASES_NAME  *****
     ---------------------------------------------------------
              has been build in $BUILDTIME!
Please Check it with Your Board!
======================================================================
EOF

exit 0


