/** @file

MODULE						: LastRunLevelTypeStruct

TAG								: 

FILE NAME					: LastRunLevelTypeStruct.h

DESCRIPTION				: A last-run-level structure definition.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _LASTRUNLEVELTYPESTRUCT_H
#define _LASTRUNLEVELTYPESTRUCT_H

typedef struct _LastRunLevelType
{
	int last;
	int run;
	int level;
	int numBits;
	int codeWord;
} LastRunLevelType;

#endif	// end _LASTRUNLEVELTYPESTRUCT_H.
