# rule for every directory

# support PC environments
#ARCH=
#ARCH=arm

# release or debug version. 2007.03.15
# must be 'release' or 'debug'
EDITION=debug
#EDITION=release

# be SN_SN926 or FIC8120
# PLATFORM=FIC8120
PLATFORM=SN_SN926

ifeq ($(ARCH),arm)
	C_FLAGS += -D__ARM_IXP__=1 
	CXXFLAGS = -D__ARM_IXP__=1 
	CROSS_COMPILER=arm-linux-
	LDFLAGS+=  
	flag=
	C_FLAGS +=-DARM
	CFLAGS+=-D$(PLATFORM)
else
	ARCH=X86
	C_FLAGS +=-D$(ARCH)
	EXTENSION=.exe
endif

C_FLAGS+=-DROOT_DIR=\"$(ROOT_DIR)\"

CC	= $(CROSS_COMPILER)gcc
CXX 	= $(CROSS_COMPILER)g++ 
STRIP	= $(CROSS_COMPILER)strip
LD	= $(CROSS_COMPILER)ld
RANLIB 	= $(CROSS_COMPILER)ranlib
STRIP 	= $(CROSS_COMPILER)strip
AR 	= $(CROSS_COMPILER)ar

RM	= rm -r -f
MKDIR	= mkdir -p
MODE	= 700
OWNER	= root
CHOWN	= chown
CHMOD	= chmod
COPY	= cp
MOVE	= mv

LN		= ln -sf

#BUILDTIME := $(shell TZ=UTC date -u "+%Y_%m_%d-%H_%M")
BUILDTIME := $(shell TZ=CN date -u "+%Y_%m_%d")
GCC_VERSION := $(shell $(CC) -dumpversion )
