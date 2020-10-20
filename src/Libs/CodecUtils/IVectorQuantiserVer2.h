/** @file

MODULE						: IVectorQuantiserVer2

TAG								: IVQV2

FILE NAME					: IVectorQuantiserVer2.h

DESCRIPTION				: A IVectorQuantiserVer2 Interface as an abstract base class to 
										vector quantiser implementations. This interface is used for 
                    vector quantisers that do not own the codebook. The codebook is
                    passed as a parameter referencing an IVQCodebook interface.

REVISION HISTORY	:
									: 

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _IVECTORQUANTISERVER2_H
#define _IVECTORQUANTISERVER2_H

#pragma once

#include "IVQCodebookDim16.h"

class IVectorQuantiserVer2
{
public:
	virtual ~IVectorQuantiserVer2() {}

	/** Create and destroy private mem objects.
	Methods to alloc and delete memory that is best not
	done in the constructor as failure to alloc cannot 
	be reported. (Try-catch trap is possible).
	@return	: 1 = success, 0 = failure.
	*/
	virtual int		Create(void)	= 0;
	virtual void	Destroy(void) = 0;

	/** Quantise the input vector.
	Match the input vector with the implementation code book element
	that produces the smallest distortion.
	@param vector			:	Input vector to match in the code book.
	@param distortion	:	Distortion of the winning vector.
  @param codebook   : Use this code book.
	@return						: Winning vector index.
	*/
	virtual int Quantise(const void* vector, int* distortion, IVQCodebookDim16* codebook) = 0;

	/** Inverse quantise the index.
	Return a pointer to the code book vector referenced by the input
	index.
	@param index	  : Code book vector index.
  @param codebook : Use this code book.
	@return				  : Reference to the code book vector.
	*/
	virtual const void* InverseQuantise(int index, IVQCodebookDim16* codebook) = 0;

};// end class IVectorQuantiserVer2.

#endif	//_IVECTORQUANTISERVER2_H
