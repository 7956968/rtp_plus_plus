/** @file

MODULE						: IInverseDct

TAG								: IID

FILE NAME					: IInverseDct.h

DESCRIPTION				: An interface to inverse Dct implementations. The interface
										has 2 options as either in-place or from an input to an
										output parameter argument. The implementations must define
										the mem type (e.g. short, int) and whether it is a 2-D idct
										on a block or 1-D on an array.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _IINVERSEDCT_H
#define _IINVERSEDCT_H

/*
---------------------------------------------------------------------------
	Interface definition.
---------------------------------------------------------------------------
*/
class IInverseDct
{
	public:
		virtual ~IInverseDct() {}
		
		/** In-place inverse Dct.
		The inverse Dct is performed on the input and replaces it with the coeffs.
		@param p	: Data to transform.
		@return		:	none.
		*/
		virtual void idct(void* ptr) = 0;

		/** Transfer inverse Dct.
		The inverse Dct is performed on the coeffs and are written to 
		the output.
		@param pCoeff	: Input coeffs.
		@param pOut		: Output data.
		@return				:	none.
		*/
		virtual void idct(void* pCoeff, void* pOut) = 0;

};//end IInverseDct.


#endif	// _IINVERSEDCT_H
