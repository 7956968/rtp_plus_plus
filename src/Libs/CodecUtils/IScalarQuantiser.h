/** @file

MODULE						: IScalarQuantiser

TAG								: ISQ

FILE NAME					: IScalarQuantiser.h

DESCRIPTION				: A IScalarQuantiser interface as an abstract base class 
										to scalar quantiser implementations that operate on 
										blocks. Simple scalar implementations can of blocks
										that consist of only 1 element.

REVISION HISTORY	:	21/11/2006 (KF): Added a Set/Get mode capability to
										allow flexibility in the implementations.

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _ISCALARQUANTISER_H
#define _ISCALARQUANTISER_H

#pragma once

class IScalarQuantiser
{
public:
	virtual ~IScalarQuantiser() {}

	/** Quantise the input block.
	Quantise the input block with the quantisation parameter 
	supplied.
	@param block	:	Block to quantise in place.
	@param quant	:	Quantisation parameter.
	@return				: none.
	*/
	virtual void quantise(void* block, int quant) = 0;

	/** Inverse quantise the block.
	Inverse quantise the input block with the quantisation 
	parameter supplied.
	@param block	:	Block to reconstruct in place.
	@param quant	:	Quantisation parameter.
	@return				: none.
	*/
	virtual void inverseQuantise(void* block, int quant) = 0;

	/** Set the mode of operation.
	Allow an implementation to define modes of operation or
	interpretation of the quantisation process.
	@param mode	: Mode to set.
	@return			: none.
	*/
	virtual void SetMode(int mode) = 0;
	virtual int  GetMode(void) = 0;

};// end class IScalarQuantiser.

#endif	//_ISCALARQUANTISER_H
