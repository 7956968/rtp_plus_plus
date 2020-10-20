/** @file

MODULE						: IVectorQuantiser

TAG								: IVQ

FILE NAME					: IVectorQuantiser.h

DESCRIPTION				: A IVectorQuantiser Interface as an abstract base class to 
										vector quantiser implementations. NOTE: After 
										instantiation of an implementation object Create() 
										must be called.

COPYRIGHT					: (c)CSIR 2013-2014 All rights reserved

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _IVECTORQUANTISER_H
#define _IVECTORQUANTISER_H

#pragma once

class IVectorQuantiser
{
public:
	virtual ~IVectorQuantiser() {}

	/** Create and destroy private mem objects.
	Methods to alloc and delete memory that is best not
	done in the constructor as failure to alloc cannot 
	be reported. (Try-catch trap is possible).
	@return	: 1 = success, 0 = failure.
	*/
	virtual int		Create(void)	= 0;
	virtual void	Destroy(void) = 0;

	/** Get the implementation vector dimension.
	This is defined as a constant of the code book.
	@return	: Vector dimension.
	*/
	virtual int	GetDimension(void) = 0;

	/** Get the number of vectors in the code book.
	This is defined as a constant of the code book.
	@return	: Code book length.
	*/
	virtual int GetCodeBookLength(void) = 0;

	/** Quantise the input vector.
	Match the input vector with the implementation code book element
	that produces the smallest distortion.
	@param vector			:	Input vector to match in the code book.
	@param distortion	:	Distortion of the winning vector.
	@return						: Winning vector index.
	*/
	virtual int Quantise(const void* vector, int* distortion) = 0;

	/** Inverse quantise the index.
	Return a pointer to the code book vector referenced by the input
	index.
	@param index	: Code book vector index.
	@return				: Reference to the code book vector.
	*/
	virtual const void* InverseQuantise(int index) = 0;

  /// Optional interface methods.
public:
  /** Select the codebook.
  Allow the vector quantiser object to encapsulate multiple codebooks.
  @param codebook : Activate this codebook.
  @return         : none
  */
  virtual void SetCodebook(int codebook) {}

};// end class IVectorQuantiser.

#endif	//_IVECTORQUANTISER_H
