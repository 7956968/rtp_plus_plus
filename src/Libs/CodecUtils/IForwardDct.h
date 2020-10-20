/** @file

MODULE						: IForwardDct

TAG								: IFD

FILE NAME					: IForwardDct.h

DESCRIPTION				: An interface to forward Dct implementations. The interface
										has 2 options as either in-place or from an input to an
										output parameter argument. The implementations must define
										the mem type (e.g. short, int) and whether it is a 2-D dct
										on a block or 1-D on an array.

REVISION HISTORY	:	

COPYRIGHT					: (c)CSIR, Meraka Institute 2007-2009 all rights resevered

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of CSIR and is released under the OPEN BSD
										license.
===========================================================================
*/
#ifndef _IFORWARDDCT_H
#define _IFORWARDDCT_H

/*
---------------------------------------------------------------------------
	Interface definition.
---------------------------------------------------------------------------
*/
class IForwardDct
{
	public:
		virtual ~IForwardDct() {}
		
		/** In-place forward Dct.
		The Dct is performed on the input and replaces it with the coeffs.
		@param ptr	: Data to transform.
		@return			:	none.
		*/
		virtual void dct(void* ptr) = 0;

		/** Transfer forward Dct.
		The Dct is performed on the input and the coeffs are written to 
		the output.
		@param pIn		: Input data.
		@param pCoeff	: Output coeffs.
		@return				:	none.
		*/
		virtual void dct(void* pIn, void* pCoeff) = 0;

};//end IForwardDct.


#endif	// _IFORWARDDCT_H
