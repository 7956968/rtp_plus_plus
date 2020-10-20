/** @file

MODULE						: IntraDctQuantiserImpl

TAG								: IDQI

FILE NAME					: IntraDctQuantiserImpl.h

DESCRIPTION				: A class to implement a scalar quantiser on the 2-D 
										dct coeffs of an Intra coded 8x8 block as defined
										in the H.263 standard. It implements the 
										IScalarQuantiser interface.

REVISION HISTORY	:
									: 

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTRADCTQUANTISERIMPL_H
#define _INTRADCTQUANTISERIMPL_H

#pragma once

#include "IScalarQuantiser.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class IntraDctQuantiserImpl : public IScalarQuantiser
{
	public:
		IntraDctQuantiserImpl()	{ _mode = 0; }
		virtual ~IntraDctQuantiserImpl()	{ }

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

	virtual void SetMode(int mode) { _mode = mode; }
	virtual int  GetMode(void)	{ return(_mode); }

protected:
	int _mode;	// 0 = clipping[-127..127], 1 = Full range clipping.

};// end class IntraDctQuantiserImpl.

#endif	//_INTRADCTQUANTISERIMPL_H
