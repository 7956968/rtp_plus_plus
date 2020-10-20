/** @file

MODULE						: StreamSwitcher

TAG								: SS

FILE NAME					: StreamSwitcher.cpp

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
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include "StreamSwitcher.h"

StreamSwitcher::StreamSwitcher()
{
	/// Reset all members.
	_state		= SS_STEADY_STATE;
	_switched	= 0;

	/// Create an input stream header reader.
	_pHeader	= NULL;
	_pHeader	= new H263v2SwitchCodecHeader();

}//end constructor.

StreamSwitcher::~StreamSwitcher()
{
	if(_pHeader != NULL)
		delete _pHeader;
	_pHeader = NULL;
}//end destructor.

/** Switch from the current stream to another stream.
This is the primary method for this class. A non-NULL stream in 
the ToStream parameter initiates the switching process. A Reset()
and NULL for the toStream params are required to hold the CurrStream 
as the output. The output is one of the dual encodings in the 
input streams. It is NOT the same type as the input. A NULL value for
the CurrStream is valid and implies switching from an empty stream.
@param pCurrStream	: Current steady state dual switchable stream.
@param currBitLen		: Length of CurrStream in bits.
@param pToStream		: Dual switchable stream to switch to.
@param toBitLen			: Length of ToStream in bits.
@param retBitLen		: Returned stream length in bits.
@return							: Single stream encoding.
*/
void* StreamSwitcher::Switch(void* pCurrStream, int currBitLen, void* pToStream, int toBitLen, int* retBitLen)
{
	if(_pHeader == NULL)
		return(NULL);

	void*	pRet = NULL;
	*retBitLen = 0;
	int		len	 = 0;

	switch(_state)
	{
		case SS_STEADY_STATE:
			/// Steady state requires the main encoding in the current stream
			/// to be output.
			{
				/// Get the current stream data as default.
				if(pCurrStream != NULL)
				{
					if(!_pHeader->Extract(pCurrStream, currBitLen))
						return(NULL);
					pRet = _pHeader->GetMainStreamPtr();
					len	 = 8 * _pHeader->GetMainStreamByteSize();
				}//end if pCurrStream...

				if(pToStream != NULL)	///< Indicates entering the switching states.
				{
					/// Get the to stream data for examination.
					if(!_pHeader->Extract(pToStream, toBitLen))
						return(NULL);
					/// If there happens to be a key I-frame in the main encoding then
					/// switch to it as a short cut to completion.
					if(_pHeader->GetMainStreamPicType() == 0)
					{
						pRet		= _pHeader->GetMainStreamPtr();
						len			= 8 * _pHeader->GetMainStreamByteSize();
						_state	= SS_SWITCHED_STATE;
					}//end if GetMainStreamPicType...
					else
					{
						/// Switch to the switching encoding on an I-frame.
						if(_pHeader->GetSwitchStreamPicType() == 0)
						{
							pRet		= _pHeader->GetSwitchStreamPtr();
							len			= 8 * _pHeader->GetSwitchStreamByteSize();
							_state	= SS_WAIT_FOR_SWITCH;
						}//end if GetSwitchStreamPicType...
						else
							_state = SS_WAIT_FOR_KEY;
					}//end else...
				}//end if pToStream...
			}//end local block...
			break;
		case SS_WAIT_FOR_KEY:
			/// Wait for an I-frame in either the stream being switched to, as a
			/// short cut, or an I-frame in the switching encoding of the to stream.
			if(pToStream == NULL)	///< Must not be NULL during the switching process.
				return(NULL);
			{
				/// Get the main encoding of the current stream data as default.
				if(pCurrStream != NULL)
				{
					if(!_pHeader->Extract(pCurrStream, currBitLen))
						return(NULL);
					pRet = _pHeader->GetMainStreamPtr();
					len	 = 8 * _pHeader->GetMainStreamByteSize();
				}//end if pCurrStream...

				/// Get the ToStream data for examination.
				if(!_pHeader->Extract(pToStream, toBitLen))
					return(NULL);
				/// If there happens to be a key I-frame in the main encoding then
				/// switch to it as a short cut to completion.
				if(_pHeader->GetMainStreamPicType() == 0)
				{
					pRet		= _pHeader->GetMainStreamPtr();
					len			= 8 * _pHeader->GetMainStreamByteSize();
					_state	= SS_SWITCHED_STATE;
				}//end if GetMainStreamPicType...
				else
				{
					/// Switch to the switching encoding on an I-frame.
					if(_pHeader->GetSwitchStreamPicType() == 0)
					{
						pRet		= _pHeader->GetSwitchStreamPtr();
						len			= 8 * _pHeader->GetSwitchStreamByteSize();
						_state	= SS_WAIT_FOR_SWITCH;
					}//end if GetSwitchStreamPicType...
				}//end else...
			}//end local block...
			break;
		case SS_WAIT_FOR_SWITCH:
			/// The switching encoding of the ToStream is output up until an
			/// I-frame is detected where the main encoding is then output. An
			/// I-frame in the main encoding is still checked as a short cut.
			if(pToStream == NULL)	///< Must not be NULL during the switching process.
				return(NULL);
			{
				/// Get the switching encoding of the ToStream data as default.
				if(!_pHeader->Extract(pToStream, toBitLen))
					return(NULL);
				pRet = _pHeader->GetSwitchStreamPtr();
				len	 = 8 * _pHeader->GetSwitchStreamByteSize();

				/// If there happens to be a key I-frame in the main encoding then
				/// switch to it as a short cut to completion. Or switch to the main
				/// encoding on an I-frame in the switching encoding.
				if((_pHeader->GetMainStreamPicType() == 0)||(_pHeader->GetSwitchStreamPicType() == 0))
				{
					pRet		= _pHeader->GetMainStreamPtr();
					len			= 8 * _pHeader->GetMainStreamByteSize();
					_state	= SS_SWITCHED_STATE;
				}//end if GetMainStreamPicType...
			}//end local block...
			break;
		case SS_SWITCHED_STATE:
			/// Output the main encoding of the ToStream stream.
			if(pToStream == NULL)	///< Must not be NULL during the switching process.
				return(NULL);
			if(!_pHeader->Extract(pToStream, toBitLen))
				return(NULL);
			pRet		= _pHeader->GetMainStreamPtr();
			len			= 8 * _pHeader->GetMainStreamByteSize();
			break;
	}//end switch _state...

	/// Set the switched state.
	if(_state == SS_SWITCHED_STATE)
		_switched = 1;
	else
		_switched = 0;

	*retBitLen = len;
	return(pRet);
}//end Switch.


