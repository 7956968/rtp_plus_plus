/** @file

MODULE						: FastInverseDctImpl1

TAG								: FIDI1

FILE NAME					: FastInverseDctImpl1.h

DESCRIPTION				: A class to implement a fast inverse 8x8 2-D dct on the 
										input. It implements the IInverseDct interface. The 
										scaling is designed for use in H.263 codecs.

REVISION HISTORY	:
									: 

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _FASTINVERSEDCTIMPL1_H
#define _FASTINVERSEDCTIMPL1_H

#pragma once

#include "IInverseDct.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastInverseDctImpl1 : public IInverseDct
{
	public:
		FastInverseDctImpl1()	{ }
		virtual ~FastInverseDctImpl1()	{ }

	// Interface implementation.
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

};// end class FastInverseDctImpl1.

#endif	//_FASTFORWARDDCTIMPL1_H
