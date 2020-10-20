/** @file

MODULE						: IReEntry

TAG								: IRE

FILE NAME					: IReEntry.h

DESCRIPTION				: A IReEntry Interface as an abstract base class to video
										codecs that are not COM based. This interface defines
										the reentry encoding required for stream switching. Each
										method must be called after its associated base codec
										method.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2006  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _IREENTRY_H
#define _IREENTRY_H

#pragma once

class IReEntry
{
public:
	virtual ~IReEntry() {}

	/** Get the reentry coded bit length.
	This indicates the exact no. of bits/bytes in the reentry coded stream.
	@return	: The length.
	*/
	virtual int GetReCompressedBitLength(void) = 0;
	virtual int GetReCompressedByteLength(void) = 0;

	/** Restart the reentry codec.
	Used to generate key frames/pages/packets that have no prior
	dependencies on previously coded frames. Resets any prediction
	loops.
	@return	: none.
	*/
	virtual void RestartRe(void) = 0;

	/** Open the reentry codec.
	Codec is opened with the parameter set described by the result
	of several SetParameter() calls. Defaults must be appropriate.
	@return	: 1 = success, 0 = failure.
	*/
	virtual int OpenRe(void) = 0;

	/** Close an opened reentry codec.
	Clear all memory allocations and reset the parameters to defaults.
	@return	: 1 = success, 0 = failure.
	*/
	virtual int CloseRe(void) = 0;
	
	/** Encode the reentry point.
	@param pCmp	: Dest of the encoding.
	The source of the encoding is tied to the primary video codec.
	The codeParameter input usually describes the allowed number
	of bits for the encoding.
	@param codeParameter	: Codec dependent interpretation.
	@return								: 1 = success, 0 = failure.
	*/
	virtual int CodeRe(void* pCmp, int codeParameter) = 0;

};// end class IReEntry.

#endif	//_IREENTRY_H
