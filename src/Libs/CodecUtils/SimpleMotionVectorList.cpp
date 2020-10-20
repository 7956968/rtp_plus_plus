/** @file

MODULE						: SimpleMotionVectorList

TAG								: MVL

FILE NAME					: SimpleMotionVectorList.cpp

DESCRIPTION				: A class to contain motion estimation results in a
										contiguous byte mem array. The structure of the
										motion vectors is determined by its type as either
										simple or complex. Each coordinate and type element
										is 1 byte size.

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

#include <stdlib.h>

#include	"SimpleMotionVectorList.h"

/** Constructor
Construct a SIMPLE type 2D vector list of signed bytes to represent
a motion vector list.
*/
SimpleMotionVectorList::SimpleMotionVectorList(void) : 
												VectorList(VectorList::SIMPLE, 2, sizeof(char))
{
}//end constructor.

SimpleMotionVectorList::~SimpleMotionVectorList(void)
{
}//end destructor.





