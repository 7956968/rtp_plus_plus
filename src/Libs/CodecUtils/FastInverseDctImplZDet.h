/** @file

MODULE						: FastInverseDctImplZDet

TAG								: FIDIZD

FILE NAME					: FastInverseDctImplZDet.h

DESCRIPTION				: A class to implement a fast inverse 8x8 2-D dct on the 
										input. Zero coeff detection is performed before the 
										transform to reduce the number of operations.It 
										implements the IInverseDct interface. The scaling is 
										designed for use in H.263 codecs.

COPYRIGHT					: (c)CSIR 2007-2010 all rights resevered

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of CSIR, Meraka Institute and has been 
										classified as CONFIDENTIAL.
===========================================================================
*/
#ifndef _FASTINVERSEDCTIMPLZDET_H
#define _FASTINVERSEDCTIMPLZDET_H

#pragma once

#include "IInverseDct.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastInverseDctImplZDet : public IInverseDct
{
	public:
		FastInverseDctImplZDet()	{ }
		virtual ~FastInverseDctImplZDet()	{ }

	/// Interface implementation.
	public:
		/** In-place inverse Dct.
		The inverse Dct is performed on the input and replaces it with the coeffs.
		@param p	: Data to transform.
		@return		:	none.
		*/
		virtual void idct(void* ptr);

		/** Transfer inverse Dct.
		The inverse Dct is performed on the coeffs and are written to 
		the output. Not implemented.
		@param pCoeff	: Input coeffs.
		@param pOut		: Output data.
		@return				:	none.
		*/
		virtual void idct(void* pCoeff, void* pOut) { }

	/// Private methods.
	protected:
		/** Get the zero pattern in the block.
		Each 4x4 block within the higher freq coeffs of the 8x8 block is examined 
		for the existence of non-zero coeffs. 
		@param ptr	: Coeffs.
		@return			: 0 = all non-zero, 1 = Quad123, 2 = Quad13, 3 = Quad23, 4 = Quad3.
		*/
		virtual int GetPattern(void* ptr);

		/** Do the IDct assuming no zero coeffs quadrants.
		No operations are elliminated from the transform assuming this pattern. 
		@param ptr	: Coeffs.
		@return			: none.
		*/
		virtual void Quad0123Pattern(void* ptr);

		/** Do the IDct assuming quad 1, 2 and 3 are zero coeffs.
		Operations are elliminated from the transform assuming this pattern. 
		@param ptr	: Coeffs.
		@return			: none.
		*/
		virtual void Quad123Pattern(void* ptr);

		/** Do the IDct assuming quad 1 and 3 are zero coeffs.
		Operations are elliminated from the transform assuming this pattern. 
		@param ptr	: Coeffs.
		@return			: none.
		*/
		virtual void Quad13Pattern(void* ptr);

		/** Do the IDct assuming quad 2 and 3 are zero coeffs.
		Operations are elliminated from the transform assuming this pattern. 
		@param ptr	: Coeffs.
		@return			: none.
		*/
		virtual void Quad23Pattern(void* ptr);

		/** Do the IDct assuming quad 3 has zero coeffs.
		Operations are elliminated from the transform assuming this pattern. 
		@param ptr	: Coeffs.
		@return			: none.
		*/
		virtual void Quad3Pattern(void* ptr);

};// end class FastInverseDctImplZDet.

#endif	//_FASTINVERSEDCTIMPLZDET_H
