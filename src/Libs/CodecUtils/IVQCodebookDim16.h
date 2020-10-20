/** @file

MODULE						: IVQCodebookDim16

TAG								: IVQCD16

FILE NAME					: IVQCodebookDim16.h

DESCRIPTION				: A IVQCodebookDim16 Interface as an abstract base class to vector 
                    quantiser codebook implementations with vectors of size 16 shorts. 
                    This interface is used to access the codebook holding class 
                    implementations.

REVISION HISTORY	:
									: 

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _IVQCODEBOOKDIM16_H
#define _IVQCODEBOOKDIM16_H

#pragma once

class IVQCodebookDim16
{
public:
	virtual ~IVQCodebookDim16() {}

	/** Create and destroy private mem objects.
	Methods to alloc and delete memory that is best not
	done in the constructor as failure to alloc cannot 
	be reported. (Try-catch trap is possible).
	@return	: 1 = success, 0 = failure.
	*/
	virtual int		Create(void)	= 0;
	virtual void	Destroy(void) = 0;

	/** Get the number of vectors in the code book.
	This is defined as a constant of the code book.
	@return	: Code book length.
	*/
	virtual int GetCodeBookLength(void) = 0;

	/** Get a reference to the 2-D the code book.
	This is defined as a constant of the code book.
	@return	: Code book reference.
	*/
  virtual const short (*GetCodeBook(void))[16] = 0;

	/** Get a reference to the list of code book frequencies of occurrence.
	This is defined as a constant of the code book.
	@return	: Frequency list reference.
	*/
  virtual void* GetFrequencies(void) = 0;

};// end class IVQCodebookDim16.

#endif	//_IVQCODEBOOKDIM16_H
