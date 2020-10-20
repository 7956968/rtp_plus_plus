/** @file

MODULE						: LastRunLevelH263List

TAG								: LRLH263L

FILE NAME					: LastRunLevelH263List.cpp

DESCRIPTION				: A class to hold H.263 last-run-level 8x8 block data.
										Operations on the data are defined elsewhere.

REVISION HISTORY	:
									: 

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include "LastRunLevelH263List.h"

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
LastRunLevelH263List::LastRunLevelH263List(void)
{
	// Dangerous to alloc mem in the constructor. Should be 2 phase 
	// construction. Use ready flag to check against.
	_ready				= 0;
	_numElements	= 0;

	// The 8x8 blocks can have a max of 64 structs.
	_pList = NULL;
	_pList = new LastRunLevelH263[64];

	if(_pList != NULL)
		_ready = 1;

}//end constructor.

LastRunLevelH263List::~LastRunLevelH263List(void)
{
	if(_pList != NULL)
		delete[] _pList;
	_pList = NULL;

	_ready = 0;	// Not really necessary.
}//end destructor.


