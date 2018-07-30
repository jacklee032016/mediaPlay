
// crt_dup.c
// This program uses the variable old to save
// the original stdout. It then opens a new file named
// DataFile and forces stdout to refer to it. Finally, it
// restores stdout to its original state.
//

#include <io.h>
#include <stdlib.h>
#include <stdio.h>

int main( void )
{
	int old;
	FILE *DataFile;

	/* old and 1 refer to same fd, eg. stdout */
	old = _dup( 1 ); /* "old" now refers to "stdout"  Note:  file descriptor 1 == "stdout" */
	if( old == -1 )
	{
		perror( "_dup( 1 ) failure" );
		exit( 1 );
	}
	
	_write( old, "This goes to stdout first\n", 26 );
	
	if( fopen_s( &DataFile, "data", "w" ) != 0 )
	{
		puts( "Can't open file 'data'\n" );
		exit( 1 );
	}

	/* stdout now refers to file "data" , the old stdout is closed on fd 1 */
	if( -1 == _dup2( _fileno( DataFile ), 1 ) )
	{
		perror( "Can't _dup2 stdout" );
		exit( 1 );
	}
	puts( "This goes to file 'data'\n" );

	// Flush stdout stream buffer so it goes to correct file 
	fflush( stdout );
	fclose( DataFile );

	// Restore original stdout 
	_dup2( old, 1 );
	puts( "This goes to stdout\n" );
	puts( "The file 'data' contains:" );
	_flushall();
	system( "type data" );

	return 0;
}

