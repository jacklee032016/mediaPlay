#!/bin/sh
# pack the build Shared into package and release it
# Jack Lee
#

DATDIR=`echo $1 | sed 's/\/\//\//g'`
PKGDIR="$DATDIR/../pkg"

	echo ""
	echo ""
	echo "Install Shared Library and EXEs for "$ARCH
	echo "Build Directris in $PKGDIR"
	
	mkdir -p $PKGDIR
	mkdir -p $PKGDIR/lib
	mkdir -p $PKGDIR/usr/bin
	mkdir -p $PKGDIR/sbin
	
	echo ""
#	echo "   Copy Configuration and Audio Data into $PKGDIR..."
#	cp -r $SRCDIR/* $PKGDIR

	echo ""
	echo "   Copy Library into $PKGDIR..."
	EXES=`find $DATDIR -name lib*.so -prune `
	for p in $EXES; do
			f=`basename $p`
			echo "                   $f is copied..."
			cp $p $PKGDIR/lib/
	done

	echo "   Copy EXE into $PKGDIR..."
	EXES=`find $DATDIR/usr/bin/ -type f `
	for p in $EXES; do
			f=`basename $p`
			echo "                  $f is copied..."
			cp $p $PKGDIR/usr/bin
	done

	EXES=`find $DATDIR/sbin/ -type f `
	for p in $EXES; do
			f=`basename $p`
			echo "                  $f is copied..."
			cp $p $PKGDIR/sbin
	done

	CVS_FILES=`find $PKGDIR -name \.svn `
	
	for f in $CVS_FILES; do
		echo "          Remove $f"
		rm -rf $f
	done

	cd $PKGDIR
	tar czf $RELEASES_NAME *
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
