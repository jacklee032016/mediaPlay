
// crt_pipe.c
/* This program uses the _pipe function to pass streams of
 * text to spawned processes.
 */

#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <math.h>

enum PIPES { READ, WRITE }; /* Constants 0 and 1 for READ and WRITE */
#define NUMPROBLEM 8

int main( int argc, char *argv[] )
{

	int fdpipe[2];
	char hstr[20];
	int pid, problem, c;
	int termstat;

	/* If no arguments, this is the spawning process */
	if( argc == 1 )
	{
		setvbuf( stdout, NULL, _IONBF, 0 );

		/* Open a set of pipes */
		if( _pipe( fdpipe, 256, O_BINARY ) == -1 )
			exit( 1 );


		/* Convert pipe read descriptor to string and pass as argument 
		* to spawned program. Program spawns itself (argv[0]).
		*/
		_itoa_s( fdpipe[READ], hstr, sizeof(hstr), 10 );
		if( ( pid = _spawnl( P_NOWAIT, argv[0], argv[0], hstr, NULL ) ) == -1 )
			printf( "Spawn failed" );

		/* Put problem in write pipe. Since spawned program is 
		* running simultaneously, first solutions may be done 
		* before last problem is given.
		*/
		for( problem = 1000; problem <= NUMPROBLEM * 1000; problem += 1000)
		{
			printf( "Son, what is the square root of %d?\n", problem );
			_write( fdpipe[WRITE], (char *)&problem, sizeof( int ) );
		}

		/* Wait until spawned program is done processing. */
		_cwait( &termstat, pid, WAIT_CHILD );
		if( termstat & 0x0 )
			printf( "Child failed\n" );

		_close( fdpipe[READ] );
		_close( fdpipe[WRITE] );
	}
	else
	{/* If there is an argument, this must be the spawned process. */
		/* Convert passed string descriptor to integer descriptor. */
		fdpipe[READ] = atoi( argv[1] );

		/* Read problem from pipe and calculate solution. */
		for( c = 0; c < NUMPROBLEM; c++ )
		{
			_read( fdpipe[READ], (char *)&problem, sizeof( int ) );
			printf( "Dad, the square root of %d is %3.2f.\n",	problem, sqrt( ( double )problem ) );
		}
	}
}

