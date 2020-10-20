/** @file

MODULE						: IntraDctQuantiserImplLookUp

TAG								: IDQILU

FILE NAME					: IntraDctQuantiserImplLookUp.h

DESCRIPTION				: A class to implement a scalar quantiser on the 2-D 
										dct coeffs of an Intra coded 8x8 block as defined
										in the H.263 standard. A large look up table is used.
										This class extends the IScalarQuantiser() base class.

REVISION HISTORY	:

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTRADCTQUANTISERIMPLLOOKUP_H
#define _INTRADCTQUANTISERIMPLLOOKUP_H

#pragma once

#include "IScalarQuantiser.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class IntraDctQuantiserImplLookUp : public IScalarQuantiser
{
public:
		IntraDctQuantiserImplLookUp();
		virtual ~IntraDctQuantiserImplLookUp()	{ }

	/// Interface implementation.
public:
	/** Quantise the input block.
	Quantise the input block with the quantisation parameter 
	supplied.
	@param block	:	Block to quantise in place.
	@param quant	:	Quantisation parameter.
	@return				: none.
	*/
	virtual void quantise(void* block, int quant);

	/** Inverse quantise the block.
	Inverse quantise the input block with the quantisation 
	parameter supplied.
	@param block	:	Block to reconstruct in place.
	@param quant	:	Quantisation parameter.
	@return				: none.
	*/
	virtual void inverseQuantise(void* block, int quant);

	virtual void SetMode(int mode) { _mode = mode; }
	virtual int  GetMode(void)	{ return(_mode); }

protected:
	int _mode;	// 0 = clipping[-127..127], 1 = Full range clipping.

	/// Tables dependent on mode selection.
	short MODE0_Q[32][4096];	///< Range = [-127..127]
	short MODE0_IQ[32][256];
	short MODE1_Q[32][4096];	///< Range = [-1024..1023]
	short MODE1_IQ[32][2048];

};// end class IntraDctQuantiserImplLookUp.

#endif	//_INTRADCTQUANTISERIMPLLOOKUP_H
