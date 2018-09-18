#!/bin/sh
# pack the build Shared into package and release it
# Jack Lee
#

SAMPLE_NAME="g05.mov"

DATDIR=`echo $1 | sed 's/\/\//\//g'`
PKGDIR="$DATDIR/../pkg"
SRCDIR=`echo $2 | sed 's/\/\//\//g'`

	echo ""
	echo ""
	echo "Install Shared Library and EXEs for "$ARCH
	echo "Build Directris in $PKGDIR"
	
	VERBOSE=-v
	
	mkdir -p $PKGDIR
	mkdir -p $PKGDIR/usr/bin
	mkdir -p $PKGDIR/sbin
	mkdir -p $PKGDIR/lib
	mkdir -p $PKGDIR/etc/mLab
	mkdir -p $PKGDIR/etc/sys
	mkdir -p $PKGDIR/var/admin/cgi-bin

CONFIG_FILES="muxLab.png muxMain.conf muxPlayer.conf muxRecorder.conf 
		muxServer.conf muxWeb.conf 
		ipCmds.json mediaActions.json playActions.json 
		recordActions.json serverActions.json 
		sysAdminActions.json webActions.json 
		"
	
	echo ""
	echo "   Copy Configuration into $PKGDIR..."
	cp $VERBOSE -r $SRCDIR/etc/mime.types	$PKGDIR/etc
	cp $VERBOSE -r $SRCDIR/etc/sys	$PKGDIR/etc/sys
	for cfg in $CONFIG_FILES; do
		cp $VERBOSE -r $SRCDIR/etc/mLab/$cfg $PKGDIR/etc/mLab
	done
#	cp $VERBOSE -r $SRCDIR/etc/mLab/muxWeb.conf $PKGDIR/etc/mLab/muxWeb.conf

	echo ""
	echo "   Copy Library into $PKGDIR..."
	EXES=`find $DATDIR -name *.so -prune `
	for p in $EXES; do
			f=`basename $p`
			echo "                   $f is copied..."
			cp $p $PKGDIR/lib/
	done

	echo ""
#	echo "   Copy resources for web into $PKGDIR..."
#	cp $VERBOSE -r $SRCDIR/var	$PKGDIR/


	echo ""
	echo "   Copy EXE into $PKGDIR..."
	EXES=`find $DATDIR/usr/bin/ -type f `
	for p in $EXES; do
			f=`basename $p`
			echo "                  $f is copied..."
			cp $VERBOSE $p $PKGDIR/usr/bin
	done

	echo ""
	EXES=`find $DATDIR/sbin/ -type f `
	for p in $EXES; do
			f=`basename $p`
			echo "                  $f is copied..."
			cp $VERBOSE $p $PKGDIR/sbin
	done

	echo ""
	echo "   Copy Sample Video into $PKGDIR..."
	EXES="$SRCDIR/media/$SAMPLE_NAME"
	for p in $EXES; do
			f=`basename $p`
			echo "                  $f is copied..."
			cp $VERBOSE $p $PKGDIR/
	done

	echo ""
#	echo "   Copy CGI into $PKGDIR..."
	EXES=`find $DATDIR/var/admin/cgi-bin/ -type f `
	for p in $EXES; do
			f=`basename $p`
			echo "                  $f is copied..."
			cp $VERBOSE $p $PKGDIR/var/admin/cgi-bin/
	done


	CVS_FILES=`find $PKGDIR -name \.svn `
	
	for f in $CVS_FILES; do
		echo "          Remove $f"
		rm -rf $f
	done

	cd $PKGDIR
#	tar czf $RELEASES_NAME *
	echo "zip $RELEASES_NAME *"
	zip -r $RELEASES_NAME etc lib sbin usr $SAMPLE_NAME
	mv $RELEASES_NAME $ROOT_DIR

cat << EOF
======================================================================
Shared Library and EXEs for $ARCH are installed 
     _________________________________________________________
     *****  $RELEASES_NAME  *****
     ---------------------------------------------------------
              has been build in $BUILDTIME!
Please Check it with Your Board!
======================================================================
EOF

exit 0
