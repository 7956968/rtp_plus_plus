/** @file

MODULE						: VectorQuantiserVer2Dim16Impl1

TAG								: VQV2D16I1

FILE NAME					: VectorQuantiserVer2Dim16Impl1.h

DESCRIPTION				: A class to implement the IVectorQuantiserVer2 interface for video 
										4x4 (dim = 16) dimension values. A fast implementation with early 
                    exit short cuts. 

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)CSIR 2013-2014  all rights resevered

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _VECTORQUANTISERVER2DIM16IMPL1_H
#define _VECTORQUANTISERVER2DIM16IMPL1_H

#include "IVectorQuantiserVer2.h"

/*
===========================================================================
	Class definition.
===========================================================================
*/
class VectorQuantiserVer2Dim16Impl1 : public IVectorQuantiserVer2
{
public:
  VectorQuantiserVer2Dim16Impl1() {}
  virtual ~VectorQuantiserVer2Dim16Impl1() {}

	// IVectorQuantisation Interface.

	/** Create and destroy private mem objects.
	Methods to alloc and delete memory that is best not
	done in the constructor as failure to alloc cannot 
	be reported. (Try-catch trap is possible).
	@return	: 1 = success, 0 = failure.
	*/
  int		Create(void)  { return(1); }
  void	Destroy(void) {}

	/** Quantise the input vector.
	Match the input vector with the implementation code book element
	that produces the smallest distortion.
	@param vector			:	Input vector to match in the code book.
	@param distortion	:	Distortion of the winning vector.
  @param codebook   : Use this code book.
	@return						: Winning vector index.
	*/
	int Quantise(const void* vector, int* distortion, IVQCodebookDim16* codebook);

	/** Inverse quantise the index.
	Return a pointer to the code book vector referenced by the input
	index.
	@param index	  : Code book vector index.
  @param codebook : Use this code book.
	@return				  : Reference to the code book vector.
	*/
	const void* InverseQuantise(int index, IVQCodebookDim16* codebook);

};// end class VectorQuantiserVer2Dim16Impl1.

#endif	// _VECTORQUANTISERVER2DIM16IMPL1_H

