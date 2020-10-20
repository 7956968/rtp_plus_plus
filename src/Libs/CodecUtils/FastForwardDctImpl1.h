/** @file

MODULE						: FastForwardDctImpl1

TAG								: FFDI1

FILE NAME					: FastForwardDctImpl1.h

DESCRIPTION				: A class to implement a fast forward 8x8 2-D dct on the 
										input. It implements the IForwardDct interface. The 
										scaling is designed for use in H.263 codecs.

REVISION HISTORY	:
									: 

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _FASTFORWARDDCTIMPL1_H
#define _FASTFORWARDDCTIMPL1_H

#pragma once

#include "IForwardDct.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastForwardDctImpl1 : public IForwardDct
{
	public:
		FastForwardDctImpl1()	{ }
		virtual ~FastForwardDctImpl1()	{ }

	// Interface implementation.
	public:
		/** In-place forward Dct.
		The Dct is performed on the input and replaces it with the coeffs.
		@param p	: Data to transform.
		@return		:	none.
		*/
		virtual void dct(void* ptr);

		/** Transfer forward Dct.
		The Dct is performed on the input and the coeffs are written to 
		the output.
		@param pIn		: Input data.
		@param pCoeff	: Output coeffs.
		@return				:	none.
		*/
		virtual void dct(void* pIn, void* pCoeff);

};// end class FastForwardDctImpl1.

#endif	//_FASTFORWARDDCTIMPL1_H
