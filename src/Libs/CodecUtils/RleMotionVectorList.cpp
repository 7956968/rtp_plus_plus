/** @file

MODULE						: RleMotionVectorList

TAG								: RMVL

FILE NAME					: RLEMotionVectorList.cpp

DESCRIPTION				: A class to contain motion run-length results in a
										contiguous struct array. The structure of the
										list is defined in a struct.

REVISION HISTORY	:	

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

#include <memory.h>
#include <string.h>
#include <stdlib.h>


#include "RleMotionVectorList.h"

RleMotionVectorList::RleMotionVectorList(void)
{
	_maxLength	= 0;
	_length			= 0;
	_pList			= NULL;
}//end constructor.

RleMotionVectorList::~RleMotionVectorList(void)
{
	if(_pList != NULL)
		delete[] _pList;
	_pList = NULL;
}//end destructor.

/** Set mem size dependent on length required.
@param	maxLength	: Num of structs in _pList;
@return					: 1 = success, 0 = failure.
*/
int RleMotionVectorList::SetMaxLength(int maxLength)
{
	if(maxLength < 0)
		return(0);

	// Clear out old mem.
	if(_pList != NULL)
		delete[] _pList;
	_pList			= NULL;
	_maxLength	= 0;
	_length			= 0;	// Reset the number of valid structs.

	// Create new space.
	_pList = new RleMotionVectorType[maxLength];
	if(_pList == NULL)
		return(0);

	_maxLength = maxLength;

	return(1);
}//end SetMaxLength.





