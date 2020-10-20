/** @file

MODULE						: VectorList

TAG								: VL

FILE NAME					: VectorList.cpp

DESCRIPTION				: A generic class to contain vectors in a contiguous byte 
										mem array. Used when fast access to mem is required 
										otherwise use STL::vector type. The structure of the	
										vectors is determined by its type as either	simple or 
										complex.

REVISION HISTORY	:	29/09/2006 (KF): Added a GetPatternLoc() method to
										give random access to the head of any pattern in 
										the list.

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

#include	"VectorList.h"

/*
--------------------------------------------------------------------------
  Constants. 
--------------------------------------------------------------------------
*/
const int	VectorList::SIMPLE	= 0;
const int	VectorList::COMPLEX	= 1;

VectorList::VectorList(int type, int dim, int bytesPerElement, int maxVecsPerPattern)
{
	_dim								= dim;
	_bytesPerElement		= bytesPerElement;
	_maxVecsPerPattern	= maxVecsPerPattern;

	if(type == SIMPLE)
	{
		_type							= SIMPLE;
		_bytesPerPattern	= dim * bytesPerElement; // Ignore maxVecsPerPattern.
	}//end if SIMPLE...
	else	// ...assume COMPLEX.
	{
		_type							= COMPLEX;
		_bytesPerPattern	= (2 + (maxVecsPerPattern * dim)) * bytesPerElement;
	}//end else...

	_length							= 0;
	_size								= 0;
	_pData							= NULL;
}//end constructor.

VectorList::~VectorList(void)
{
	if(_pData != NULL)
		delete[] _pData;
	_pData = NULL;
}//end destructor.

/** Set mem size dependent on type and length of vector list.
There is no checking on the validity of the members set during construction.
@param	length	: Num of patterns in _pData;
@return					: 1 = success, 0 = failure.
*/
int VectorList::SetLength(int length)
{
	if(length < 0)
		return(0);

	// Clear out old mem.
	if(_pData != NULL)
		delete[] _pData;
	_pData	= NULL;
	_size		= 0;
	_length = 0;

	// Create new space on construction parameters.
	_pData = new char[_bytesPerPattern * length];
	if(_pData == NULL)
		return(0);

	_size		= _bytesPerPattern * length;
	_length = length;

	return(1);
}//end SetLength.

void* VectorList::GetPatternLoc(int pos)
{
	if( (_pData == NULL)||(pos >= _length) )
		return(NULL);
	unsigned char* p = (unsigned char *)_pData;
	return( &(p[pos * _bytesPerPattern]) ); 
}//end GetPatternLoc.

