/** @file

MODULE						: FastForwardDctImpl2

TAG								: FFDI2

FILE NAME					: FastForwardDctImpl2.h

DESCRIPTION				: A class to implement a fast forward 8x8 2-D dct on the 
										input. It implements the IForwardDct interface. The 
										scaling is designed for use in H.263 codecs.

COPYRIGHT					: (c)CSIR, Meraka Institute 2007-2008 all rights resevered

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of CSIR, Meraka Institute and has been 
										classified as CONFIDENTIAL.
===========================================================================
*/
#ifndef _FASTFORWARDDCTIMPL2_H
#define _FASTFORWARDDCTIMPL2_H

#pragma once

#include "IForwardDct.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastForwardDctImpl2 : public IForwardDct
{
	public:
		FastForwardDctImpl2()	{ }
		virtual ~FastForwardDctImpl2()	{ }

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

};// end class FastForwardDctImpl2.

#endif	//_FASTFORWARDDCTIMPL2_H
