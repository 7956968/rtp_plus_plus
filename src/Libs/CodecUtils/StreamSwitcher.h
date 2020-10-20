/** @file

MODULE						: StreamSwitcher

TAG								: SS

FILE NAME					: StreamSwitcher.h

DESCRIPTION				: A class to handle switching between two independent streams
										produced by SwitchCodecs. The output is a single codec 
										stream.

REVISION HISTORY	:

COPYRIGHT					: (c)CSIR, Meraka Institute 2007-2008 all rights reserved

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of CSIR, Meraka Institute and has been 
										classified as CONFIDENTIAL.
===========================================================================
*/
#ifndef _STREAMSWITCHER_H
#define _STREAMSWITCHER_H

#pragma once

#include "H263v2SwitchCodecHeader.h"

/**
---------------------------------------------------------------------------
	Stream Switching operation.
---------------------------------------------------------------------------
Note: Switch() is called on every time sequential frame of the stream.

1.	In the Reset() state the current stream is output and the Switch() method
		is called with the 'to' stream equal to NULL. IsSwitched() == 0.

2.	A call to Switch() with a non-NULL 'to' parameter initiates the switching 
		state machine.

3.	The application monitors IsSwitched() for completion.

4.	On IsSwitched(), Reset() is called and Switch() is called with the old
		'to' stream as the current stream and NULL as the 'to' stream parameter.
*/
/**
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class StreamSwitcher
{
public:
	StreamSwitcher();
	virtual ~StreamSwitcher();

/// Interface implementation.
public:
	/** Switch from the current stream to another stream.
	This is the primary method for this class. A non-NULL stream in 
	the ToStream parameter initiates the switching process. A Reset()
	and NULL for the toStream param are required to hold the CurrStream 
	as the output. The output is one of the dual encodings in the 
	input streams. It is NOT the same type as the input.
	@param pCurrStream	: Current steady state dual switchable stream.
	@param currBitLen		: Length of CurrStream in bits.
	@param pToStream		: Dual switchable stream to switch to.
	@param toBitLen			: Length of ToStream in bits.
	@param retBitLen		: Returned stream length in bits.
	@return							: Single stream encoding.
	*/
	void* Switch(void* pCurrStream, int currBitLen, void* pToStream, int toBitLen, int* retBitLen);

	/** Reset to current stream as output.
	Force the output to be the main stream of the CurrStream on 
	calls to Switch().
	@return	: none.
	*/
	void Reset(void)	{ _switched = 0; _state = SS_STEADY_STATE; }

	/** Get the switch status.
	@return	: Switch state.
	*/
	int IsSwitched(void)	{ return(_switched); }

/// Constants.
private:
	/// States.
	static const int SS_STEADY_STATE		= 0;
	static const int SS_WAIT_FOR_KEY		= 1;
	static const int SS_WAIT_FOR_SWITCH = 2;
	static const int SS_SWITCHED_STATE	= 3;

///	Status members and utilities.
private:
	/// Switch status (position).
	int	_switched;

	/// Switching state machine state.
	int _state;

	/// Utility to get info from the streams.
	H263v2SwitchCodecHeader* _pHeader;

};// end class StreamSwitcher.

#endif	// _STREAMSWITCHER_H
