/** @file

MODULE						: AdvancedIntraDctQuantiserImpl

TAG								: AIDQI

FILE NAME					: AdvancedIntraDctQuantiserImpl.h

DESCRIPTION				: A class to implement a scalar quantiser on the 2-D 
										dct coeffs of an Advanced Intra coded 8x8 block as 
										defined	in the H.263 standard. It implements the 
										IScalarQuantiser interface.

REVISION HISTORY	:
									: 

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _ADVANCEDINTRADCTQUANTISERIMPL_H
#define _ADVANCEDINTRADCTQUANTISERIMPL_H

#pragma once

#include "IScalarQuantiser.h"

typedef short pqType;

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class AdvancedIntraDctQuantiserImpl : public IScalarQuantiser
{
	public:
		AdvancedIntraDctQuantiserImpl(void);
		virtual ~AdvancedIntraDctQuantiserImpl(void);

	// Interface implementation.
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

	/** Set the mode of operation.
	No modes for this implementation.
	@param mode	: Mode to set.
	@return			: none.
	*/
	virtual void SetMode(int mode) { _mode = mode; }
	virtual int  GetMode(void)	{ return(_mode); }

protected:
	int _mode;	// 0 = clipping[-127..127], 1 = Full range clipping.

};// end class AdvancedIntraDctQuantiserImpl.

#endif	//_ADVANCEDINTRADCTQUANTISERIMPL_H
