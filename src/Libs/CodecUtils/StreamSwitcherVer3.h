/** @file

MODULE						: StreamSwitcherVer3

TAG								: SSV3

FILE NAME					: StreamSwitcherVer3.h

DESCRIPTION				: A class to handle switching between two independent streams
										produced by SwitchCodecs. The output is a single codec 
										stream. This 3rd version encapsulates references to the 
										encoded streams within the class.

COPYRIGHT					: (c)CSIR, Meraka Institute 2007-2012 all rights reserved

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of CSIR, Meraka Institute and has been 
										classified as CONFIDENTIAL.
===========================================================================
*/
#ifndef _STREAMSWITCHERVER3_H
#define _STREAMSWITCHERVER3_H

#pragma once

#include "SwitchCodecHeaderVer1.h"

/**
---------------------------------------------------------------------------
	Stream Switching operation.
---------------------------------------------------------------------------
Note: GetNextFrame() is called on every time sequential frame of the stream.

1.	In the Reset() state the current stream is output and the 'to' stream is
		equal to NULL. IsSwitched() == 1.

2.	A call to Switch() with a non-NULL 'to' parameter initiates the switching 
		state machine.

3.	The application monitors IsSwitched() for completion.

4.	On IsSwitched(), the new 'to' stream becomes the current stream and the
		old current stream is set to NULL.

5.	Note that the stream pointers must hold const buffers that do not change
		during switching. The only oportunity to change a stream is via the call
		to the Switch() method.
*/
/**
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class StreamSwitcherVer3
{
public:
	StreamSwitcherVer3(void);
	StreamSwitcherVer3(void* pInitialStream);
	int ResetMembers(void);
	virtual ~StreamSwitcherVer3();

/// Interface implementation.
public:
	/** Next frame delivered to the output of the switcher.
	The receiving application reads the next frame through this call. The
	mechanism for switching between frames is hidden behind this call. The
	application calls this every time a new frame is loaded into the mem
	referenced by _pCurrStream and/or _pNewStream. The returned encoding 
	be checked for NULL as initially, before the 1st stream is active, the
	current stream does not yet exist.
	@param retBitLen		: Returned stream length in bits.
	@return							: Single stream encoding.
	*/
	void* GetNextFrame(int* retBitLen);

	/** Switch from the current stream to a new stream.
	A non-NULL stream in the pToStream parameter initiates the switching 
	process. However, a switch can only be activated if a previous switch
	is not in progress. A single call to this method is all that is required
	and further calls will be rejected until the switch is complete.
	@param pToStream		: Dual switchable stream to switch to.
	@return							: 1 = success, 0 = switch request not accepted.
	*/
	int Switch(void* pToStream);

	/** Reset to current stream as output.
	Force the output to be the main stream of the CurrStream on 
	calls to Switch().
	@return	: none.
	*/
	void Reset(void)	{ _switched = 1; _state = SSV3_STEADY_STATE; }

	/** Get the switch status.
	@return	: Switch state.
	*/
	int IsSwitched(void)	{ return(_switched); }

/// Constants.
private:
	/// States.
	static const int SSV3_STEADY_STATE		= 0;
	static const int SSV3_WAIT_FOR_KEY		= 1;
	static const int SSV3_WAIT_FOR_SWITCH = 2;
	static const int SSV3_SWITCHED_STATE	= 3;

///	Status members and utilities.
private:
	/// Switch status (position).
	int	_switched;

	/// Switching state machine state.
	int _state;

	/// References to the current and switch to streams. These
	/// must be valid application wide const buffer pointers
	/// throughout the life of this object.
	void*	_pCurrStream;	///< Set to _pNewStream after the switch.
	void* _pNewStream;	///< Reset to NULL after the switch.

	/// Utility to get info from the streams.
	SwitchCodecHeaderVer1* _pHeader;

};// end class StreamSwitcher2.

#endif	// _STREAMSWITCHERVER3_H
