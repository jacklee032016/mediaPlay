				Build and Test for PC and embedded platform
																	May, 2017


# svn check one directory into current directory, omitting the name of SVN directory:
svn co http://10.0.1.7/workingRepos/Projects/AngelicaM/trunk/Firmware/TX/player

export file which is not in the working directory of SVN:
svn export http://10.0.1.7/workingRepos/Projects/AngelicaM/trunk/Firmware/TX/player/readme.txt
svn export http://10.0.1.7/workingRepos/Projects/AngelicaM/trunk/Firmware/TX/player/Makefile.pre
svn export http://10.0.1.7/workingRepos/Projects/AngelicaM/trunk/Firmware/TX/player/Rules.mak
svn co http://10.0.1.7/workingRepos/Projects/AngelicaM/trunk/Firmware/TX/player/muxMedia
svn co http://10.0.1.7/workingRepos/Projects/AngelicaM/trunk/Firmware/TX/player/ffmpeg

October 3rd, 2017
		Update RX board:
				Default root password: 'angelica_M_762';
				Telnet into board;
				Upload packages throught WinSCP.
				
				Start serial port login: update /etc/inittab. Refer to resource/etc/inittab
				Start static IP address or DHCP client:
						Static IP address: change /etc/init.d/S80network. Refer to resource/etc/init.d/S80network
						DHCP client: udhcpc, DHCP client from busybox
				
				After that, comments the command 'load' in /etc/init.d/rcS
				Add command lines for web server, ftp server and player in /etc/init.d/rcS
				
				Make muxPlayer run as front end: comments the 'demon' configuration for muxPlayer;
				Make sure the hard-coded IP address in WebAdmin is the IP address of board;
						

August 7th, 2017
		Optimizing the build environments:
				Macros in $REPOSITORY/Rules.mak:
						HI_CFLAGS/HI_LDFLAGS
						FFMPEG_CFLAGS/FFMPEG_LDFLAGS
						SHARED_CFLAGS/SHARED_LDFLAGS
						PLAYER_HOME
				Dependence between modules:
						whttpd : no
						ffmpeg : no
						shared : 

			ffmpeg:
						zlib comes from SDK/board/include: add into 'arm' subsirectory to refer it.

http://10.0.1.7/workingRepos/Projects/AngelicaM/trunk/Firmware/TX/player/

July 18th, 2017
		Add JSON support for all: shared library, RX Player and WebAdmin;
		Remove all files which are not used again:
				Message queue, TLV(Type/Length/Value) library and communication.

July,4th, 2017
		Build both shared and static library for ARM platform; only static library is built for X86 platform.
				ffmpeg library: both static/shared library for ARM; 
				shared library: both static/shared library for ARM;
				libcgi library: only shared library for ARM;
				Makefile.post in these 3 modules;

		Program link to shared libraries:
				webAdmin CGIs;
				standalone program for testing in shared module;
		
		Programs link to static libraries:
				muxServer and other programs which call Hisilicon libraries;
				Only change the Makefile for these programs: add link path to static libraries before the shared libraries;		

Common environment shared by all modules
		rule.mak
		Makefile.pre
	
	Notes:
		These common files are contained in shared module

Modules:
	whttps:
				Web Server module, independent module
				
	shared:
				common shared library for all modules

	webAdmin:
				web administration, dependent on shared;

				