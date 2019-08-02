/*
* $Id$
*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/resource.h>
#include <signal.h>

#include <stdarg.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <dlfcn.h>

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "ms_version.h"

static	MuxMain		_muxMain;

/* init terminal so that we can grab keys */
static struct termios _oldtty;

volatile sig_atomic_t recvSigalTerminal = FALSE;

void sig_seg(void)
{
	printf("sig_seg\n");
	fflush(stdout);
	while(1)
		;
}

static void doExit(MuxMain *muxMain)
{
	fprintf(stderr, "doExit" );
}

static void termExit(void)
{
	MUX_INFO("termExit" );
	if(_muxMain.muxLog.isDaemonized)
	{
		tcsetattr (0, TCSANOW, &_oldtty);
	}

	exit(1);
}

static void sigTermHandler(int sig)
{
//	SYSTEM_SIGNAL_EXIT();
	
	MuxPlugIn *plugin = _muxMain.plugins;

	recvSigalTerminal = TRUE;
	
	while(plugin)
	{
		if(plugin->enable)
		{
			MUX_INFO("Plugin '%s' exit...", plugin->name );
			plugin->signalExit(plugin, 1);
		}
		plugin = plugin->next;
	}

	termExit();
}

static void termInit(MuxMain	*muxMain)
{
	struct termios tty;

	tcgetattr (0, &tty);
	_oldtty = tty;

	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP |INLCR|IGNCR|ICRNL|IXON); /* input modes */
	tty.c_oflag |= OPOST;/* output modes */
	tty.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN); /* local modes */
	tty.c_cflag &= ~(CSIZE|PARENB);  /* control modes */
	tty.c_cflag |= CS8; 
	tty.c_cc[VMIN] = 1; /* special characters */
	tty.c_cc[VTIME] = 0;

	if(muxMain->muxLog.isDaemonized)
	{/* change occurs immediately*/
		if(tcsetattr (0, TCSANOW, &tty) )
		{
			fprintf(stderr, "Set Terminal failed:%s", strerror(errno) );
		}
	}

	signal(SIGINT , sigTermHandler); /* Interrupt (ANSI).  */
	signal(SIGQUIT, sigTermHandler); /* Quit (POSIX).  */
	signal(SIGTERM, sigTermHandler); /* Termination (ANSI).  */
	
	/*  register a function to be called at normal program termination */
	atexit(termExit);
	
#ifdef CONFIG_BEOS_NETSERVER
	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
#endif
}

#include <sys/wait.h>

static void _sigChldHandler(int sig)
{
	int saved_errno = errno;
	pid_t pid;
	
	do
	{
		MUX_ERROR("Signal CHLD is called");
		pid = waitpid((pid_t)(-1), 0, WNOHANG);
		if(pid> 0)
		{
			MUX_INFO("thread %d exit now", pid);
		}
		
	}while(pid> 0);
	
	errno = saved_errno;
}

int sysSignalRegisterChild(void)
{
	struct sigaction sa;
	
	sa.sa_handler = &_sigChldHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	
	if (sigaction(SIGCHLD, &sa, 0) == -1)
	{
		perror(0);
		exit(1);
		return -1;
	}

	return 0;
}

static int _muxMainAddPlugIn(MuxPlugIn *plugin, MuxMain *muxMain)
{
	char					name[CMN_NAME_LENGTH];
	int ( *callBack)(MuxPlugIn *plugin);
	int ret;
	
	void *foo;
	void* handle = dlopen (plugin->path, RTLD_NOW);
	if(handle==NULL)
	{
		MUX_ERROR("Dynamic Load %s(%s) Failed : %s", plugin->name, plugin->path, dlerror() );
		exit(1);
	}	

	foo = dlsym (handle, "init");
	if( foo ==NULL)
	{
		MUX_ERROR("Symbol not found in Dynamic Load library %s(%s) : %s",plugin->name, plugin->path, dlerror() );
		exit(1);
	}
	callBack = (int (*)(MuxPlugIn *))foo;

	memcpy(name, plugin->name, sizeof(plugin->name) );
	MUX_DEBUG("Plugin '%s' ..... ", name );
	ret = callBack(plugin );
	MUX_INFO("Plugin '%s' (%s %s) ended!\n", name, plugin->name, plugin->version );

	return ret;
}

static int _muxMainReportEvent(MuxMain *muxMain, MUX_PLUGIN_TYPE pluginType, CMN_PLAY_JSON_EVENT *jEvent)
{
	MuxPlugIn	*plugin = NULL;

	plugin = muxMain->plugins;
	while(plugin)
	{
		if(plugin->type == pluginType)
		{
			if(!plugin->reportEvent)
			{
				MUX_WARN("No report event interface in plugin '%s'", plugin->name);
			}
			else
			{
				plugin->reportEvent(plugin, jEvent);

				return EXIT_SUCCESS;
			}
		}
		plugin = plugin->next;
	}
	
	MUX_WARN("Event of Plugin Type %d is not handled", pluginType);
	return EXIT_FAILURE;
}

/* return thread ID or -1 when error */
static int _muxMainStartThreadEvent(MuxMain *muxMain, CmnThread *thread, void *data)
{
	int res = EXIT_FAILURE;
	
	MUX_DEBUG("Start thread '%s' .....", thread->name );
	res = cmnThreadInit( thread, data);
	if(res == EXIT_SUCCESS)
	{
		MuxThread *muxThread = cmn_malloc(sizeof(MuxThread));
		if(muxThread == NULL)
		{
			abort();
		}
		muxThread->thread = thread;

		ADD_ELEMENT(muxMain->threads, muxThread);
//		return (int )thread->id;
		return 1;
	}

	return -EXIT_FAILURE;
}

static int _muxMainRegisterConsumer(MuxMain *muxMain, MuxMediaConsumer *mediaConsumer, char *captureName)
{
	MuxMediaCapture *mediaCapture = muxMain->mediaCaptures;
	
	if(!mediaCapture)
	{
		MUX_WARN("No media capturer is available, register consumer '%s' failed", mediaConsumer->name);
		return EXIT_FAILURE;
	}
	
	
	while(mediaCapture)
	{
		if(!strcasecmp(mediaCapture->name, captureName))
		{
			ADD_ELEMENT(mediaCapture->mediaConsumers, mediaConsumer);
			mediaConsumer->mediaCapture = mediaCapture;
			mediaCapture->nbConsumers++;

			MUX_INFO("Media consumer of '%s' is registered in Media Capturer '%s'!", mediaConsumer->name, captureName);
			return EXIT_SUCCESS;
		}
		
		mediaCapture = mediaCapture->next;
	}

	MUX_WARN("No media capturer of name '%s' is available, register consumer '%s' failed", captureName, mediaConsumer->name);
	return EXIT_FAILURE;
}

static int _muxMainAddCapture(MuxMain *muxMain, MuxMediaCapture *mediaCapture)
{
	MuxMediaCapture *_tmp = muxMain->mediaCaptures;
	int ret = EXIT_SUCCESS;

	while(_tmp)
	{
		if(!strcasecmp(_tmp->name, mediaCapture->name) )
		{
			CMN_ABORT("Media capturer with name of '%s' has been registered, so add media capture '%s' failed", mediaCapture->name);
			return EXIT_FAILURE;
		}

		_tmp = _tmp->next;
	}

	mediaCapture->muxMain = muxMain;
	ADD_ELEMENT(muxMain->mediaCaptures, mediaCapture);
	if(mediaCapture->createMediaDescribers )
	{
		ret = mediaCapture->createMediaDescribers(mediaCapture);
	}
	
	return ret;
}


int main(int argc, char **argv)
{
//	struct sigaction sigact;
	MuxMain		*muxMain = &_muxMain;
	MuxPlugIn	*plugin = NULL;
	MuxThread 	*muxThread = NULL;
	MuxMediaCapture *mediaCapture = NULL;
	int	i;
	
	int res = EXIT_SUCCESS;

	MUX_INFO(CMN_MODULE_MAIN_NAME" "CMN_VERSION_INFO(CMN_MODULE_MAIN_NAME));
	fprintf(stderr, CMN_MODULE_MAIN_NAME" " CMN_VERSION_INFO(CMN_MODULE_MAIN_NAME)"\n");

	CMN_SHARED_INIT();

	memset(muxMain, 0 , sizeof(MuxMain) );

	cmnThreadSetName(MUX_THREAD_NAME_MAIN);
//	putenv("http_proxy");
	unsetenv("http_proxy");/* Kill the http_proxy */

	srandom(cmnGetTimeMs() + (getpid() << 16));

	if( cmnMuxMainParse(MUX_MAIN_CONFIG_FILE, muxMain ) )
	{
		fprintf(stderr, "Incorrect config file : %s - exiting.\n", MUX_MAIN_CONFIG_FILE);
		exit(1);
	}
	
	snprintf(muxMain->muxLog.name, sizeof(muxMain->muxLog.name), "%s", CMN_MODULE_MAIN_NAME);
//	muxMain->mediaCaptureConfig.logLevel = muxMain->muxLog.llevel;
	muxMain->mediaCaptureConfig.boardType = muxMain->boardType;
	muxMain->mediaCaptureConfig.priv = muxMain;
	
	muxMain->reportEvent = _muxMainReportEvent;
	muxMain->initThread = _muxMainStartThreadEvent;
	muxMain->registerConsumer = _muxMainRegisterConsumer;
	muxMain->addCapture = _muxMainAddCapture;

	sysSignalRegisterChild();
	/* signal init */
	signal(SIGPIPE, SIG_IGN);
/*
	if(signal(SIGSEGV,(void *)sig_seg)==SIG_ERR)
	{
		MUX_WARN("signal SEG error\n");
	}
*/

	/* every thread must be created after set terminal attributes */
	termInit(muxMain);

	fprintf(stderr, "Start logging of '%s' .....\n", muxMain->muxLog.logFileName );
	if(cmn_log_init(&muxMain->muxLog)<0)
	{
		printf("%s Log Init Failed.\n", muxMain->muxLog.name );
		return 0;
	}

	if( cmnMuxDsValidate() )
	{
		MUX_ERROR("Security validation failed, no authority to access!");
		return 0;
	}


#if CMN_TIMER_START_DELAY
#if CMN_TIMER_DEBUG
	fprintf(stderr,"Timer inited");
#endif
	res = cmn_timer_init();
	if(res != EXIT_SUCCESS)
	{
		MUX_ERROR("timer initialization failed");
		exit(1);
	}
#endif


	cmnMediaInit(&muxMain->mediaCaptureConfig);

#if 1
	res = muxMain->initThread(muxMain, &threadCmnFtp, muxMain->mediaCaptureConfig.usbHome);
	if(res < 0 )
	{
		MUX_ERROR("failed when '%s' initializing", threadCmnFtp.name);
		exit(1);
	}

	res =  muxMain->initThread(muxMain, &threadController, muxMain);
	if(res < 0)
	{
		MUX_ERROR("failed when '%s' initializing", threadController.name );
		exit(1);
	}
#endif

	plugin = muxMain->plugins;
	while(plugin)
	{
		plugin->muxMain = muxMain;
		
		if(plugin->enable && !IS_STRING_NULL(plugin->path))
		{
//			MUX_DEBUG("Loading plugin '%s' .....", plugin->name );
			res = _muxMainAddPlugIn(plugin, muxMain);
			if( res != EXIT_SUCCESS)
			{
				MUX_ERROR("Loading plugin '%s' failed!",plugin->name );
				exit(1);
			}
		}
		plugin = plugin->next;
	}
	

//	usleep(10*1000);

#if 0
	res = muxMain->initThread(muxMain, &threadCmnFtp, muxMain->mediaCaptureConfig.usbHome);
	if(res < 0 )
	{
		MUX_ERROR("failed when '%s' initializing", threadCmnFtp.name);
		exit(1);
	}

	res =  muxMain->initThread(muxMain, &threadController, muxMain);
	if(res < 0)
	{
		MUX_ERROR("failed when '%s' initializing", threadController.name );
		exit(1);
	}
#endif

	/* start Capture thread in PLAYER and FileFeed thread in SERVER, and register thread in threadlist of muxMain */
	mediaCapture = muxMain->mediaCaptures;
	while(mediaCapture)
	{
		if(mediaCapture->startCapture )
		{
			MUX_DEBUG("Start MediaCapturer of '%s' to begin capturing.....", mediaCapture->name );
			res = mediaCapture->startCapture(mediaCapture);
			if(res != EXIT_SUCCESS)
			{
				MUX_ERROR("failed when '%s' start capturing", mediaCapture->name );
			}
//			MUX_DEBUG("MediaCapturer '%s' start capturing", mediaCapture->name );
		}

		mediaCapture = mediaCapture->next;
	}

	mediaCapture = muxMain->mediaCaptures;
	while(mediaCapture)
	{
		MuxMediaConsumer *mediaConsumer = mediaCapture->mediaConsumers;
		MUX_DEBUG("MediaCapture '%s' has %d streams, %d consumers", mediaCapture->name, mediaCapture->nbStreams, mediaCapture->nbConsumers);
		for(i=0; i< mediaCapture->nbStreams; i++)
		{
			MUX_DEBUG("\t\tNo.%d MediaStream is '%s'", i+1, mediaCapture->capturedMedias[i]->name);
		}
		
		i=0;
		while(mediaConsumer)
		{
			MUX_DEBUG("\tNo.%d MediaConsumer is '%s'", ++i, mediaConsumer->name );
			mediaConsumer = mediaConsumer->next;
		}
		mediaCapture = mediaCapture->next;
	}
	
	muxThread = muxMain->threads;
	i = 0;
	while(muxThread)
	{
//		MUX_DEBUG("No.%d thread '%s' is joined", ++i, muxThread->thread->name );
		cmnThreadJoin( muxThread->thread);
		muxThread = muxThread->next;
	}
	
	MUX_INFO("Threads exited, now free resources!" );

	plugin = muxMain->plugins;
	while(plugin)
	{
		if(plugin->enable && plugin->destory!=NULL )
		{
			plugin->destory(plugin);
		}
		plugin = plugin->next;
	}

	cmnLibDestroy();

	doExit(muxMain);

	MUX_INFO( CMN_MODULE_MAIN_NAME " Exit now!" );
	
	exit(0); /* not all OS-es handle main() return value */
	
	return 0;
}

