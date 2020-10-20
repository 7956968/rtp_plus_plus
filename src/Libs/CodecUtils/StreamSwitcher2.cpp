/** @file

MODULE						: StreamSwitcher2

TAG								: SS2

FILE NAME					: StreamSwitcher2.cpp

DESCRIPTION				: A class to handle switching between two independent streams
										produced by SwitchCodecs. The output is a single codec 
										stream. This 2nd version encapsulates references to the 
										encoded streams within the class.

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

#include "StreamSwitcher2.h"

int StreamSwitcher2::ResetMembers(void)
{
	/// Reset all members.
	_state				= SS2_STEADY_STATE;
	_switched			= 1;
	_pCurrStream	= NULL;
	_pNewStream		= NULL;

	/// Create an input stream header reader.
	_pHeader	= NULL;
	_pHeader	= new H263v2SwitchCodecHeader();
	if(_pHeader == NULL)	///< Failed to alloc mem.
		return(0);

	return(1);
}//end constructor.

StreamSwitcher2::StreamSwitcher2(void)
{
	/// Reset all members.
	ResetMembers();
}//end constructor.

StreamSwitcher2::StreamSwitcher2(void* pInitialStream)
{
	/// Reset all members.
	ResetMembers();
	/// Initialise.
	_pCurrStream = pInitialStream;
}//end constructor.

StreamSwitcher2::~StreamSwitcher2()
{
	if(_pHeader != NULL)
		delete _pHeader;
	_pHeader = NULL;
}//end destructor.

/** Next frame delivered to the output of the switcher.
This is the primary method for this class. The receiving application 
reads the next frame through this call. The mechanism for switching 
between frames is hidden behind this call. The application calls this 
every time a new frame is loaded into the mem referenced by _pCurrStream 
and/or _pNewStream. The returned encoding must be checked for NULL as 
initially, before the 1st stream is active, the current stream does 
not yet exist.
@param retBitLen		: Returned stream length in bits.
@return							: Single stream encoding.
*/
void* StreamSwitcher2::GetNextFrame(int* retBitLen)
{
	if(_pHeader == NULL)
		return(NULL);

	void*	pRet		= NULL;
	*retBitLen		= 0;
	int		len			= 0;
	int		bitLen	= _pHeader->GetHeaderBitLength();

	switch(_state)
	{
		case SS2_STEADY_STATE:
			/// Steady state requires the main encoding in the current stream
			/// to be output.
			{
				/// Get the current stream data as default.
				if(_pCurrStream != NULL)
				{
					if(!_pHeader->Extract(_pCurrStream, bitLen))
						return(NULL);
					pRet = _pHeader->GetMainStreamPtr();
					len	 = 8 * _pHeader->GetMainStreamByteSize();
				}//end if _pCurrStream...

				if(_pNewStream != NULL)	///< Indicates entering the switching states.
				{
					/// Get the new stream data for examination.
					if(!_pHeader->Extract(_pNewStream, bitLen))
						return(NULL);
					/// If there happens to be a key I-frame in the main encoding of the
					/// new stream then switch to it as a short cut to completion.
					if(_pHeader->GetMainStreamPicType() == 0)
					{
						pRet		= _pHeader->GetMainStreamPtr();
						len			= 8 * _pHeader->GetMainStreamByteSize();
						_state	= SS2_SWITCHED_STATE;
					}//end if GetMainStreamPicType...
					else
					{
						/// Switch to the new stream switching encoding on an I-frame.
						if(_pHeader->GetSwitchStreamPicType() == 0)
						{
							pRet		= _pHeader->GetSwitchStreamPtr();
							len			= 8 * _pHeader->GetSwitchStreamByteSize();
							_state	= SS2_WAIT_FOR_SWITCH;
						}//end if GetSwitchStreamPicType...
						else
							_state = SS2_WAIT_FOR_KEY;
					}//end else...
				}//end if pToStream...
			}//end local block...
			break;
		case SS2_WAIT_FOR_KEY:
			/// Wait for an I-frame in either the new stream being switched to, as a
			/// short cut, or an I-frame in the switching encoding of the new stream.
			if(_pNewStream == NULL)	///< Must not be NULL during the switching process.
				return(NULL);
			{
				/// Get the main encoding of the current stream data as default. Output
				/// the current stream while waiting for an I-frame entry point in the
				/// switching encoding of the new stream.
				if(_pCurrStream != NULL)
				{
					if(!_pHeader->Extract(_pCurrStream, bitLen))
						return(NULL);
					pRet = _pHeader->GetMainStreamPtr();
					len	 = 8 * _pHeader->GetMainStreamByteSize();
				}//end if _pCurrStream...

				/// Get the new stream data for examination.
				if(!_pHeader->Extract(_pNewStream, bitLen))
					return(NULL);
				/// If there happens to be a key I-frame in the main encoding then
				/// switch to it as a short cut to completion.
				if(_pHeader->GetMainStreamPicType() == 0)
				{
					pRet		= _pHeader->GetMainStreamPtr();
					len			= 8 * _pHeader->GetMainStreamByteSize();
					_state	= SS2_SWITCHED_STATE;
				}//end if GetMainStreamPicType...
				else
				{
					/// Switch to the switching encoding on an I-frame.
					if(_pHeader->GetSwitchStreamPicType() == 0)
					{
						pRet		= _pHeader->GetSwitchStreamPtr();
						len			= 8 * _pHeader->GetSwitchStreamByteSize();
						_state	= SS2_WAIT_FOR_SWITCH;
					}//end if GetSwitchStreamPicType...
				}//end else...
			}//end local block...
			break;
		case SS2_WAIT_FOR_SWITCH:
			/// The switching encoding of the new stream is output up until an
			/// I-frame is detected where the main encoding is then output. An
			/// I-frame in the main encoding is still checked as a short cut.
			if(_pNewStream == NULL)	///< Must not be NULL during the switching process.
				return(NULL);
			{
				/// Get the switching encoding of the new stream data as default.
				if(!_pHeader->Extract(_pNewStream, bitLen))
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
					_state	= SS2_SWITCHED_STATE;
				}//end if GetMainStreamPicType...
			}//end local block...
			break;
		case SS2_SWITCHED_STATE:
			/// Output the main encoding of the new stream stream.
			if(_pNewStream == NULL)	///< Must not be NULL during the switching process.
				return(NULL);
			if(!_pHeader->Extract(_pNewStream, bitLen))
				return(NULL);
			pRet		= _pHeader->GetMainStreamPtr();
			len			= 8 * _pHeader->GetMainStreamByteSize();

			/// The switching process is complete therefore the _pCurrStream can be
			/// discarded and the _pNewStream becomes the current stream of the
			/// steady state condition.
			_pCurrStream	= _pNewStream;
			_pNewStream		= NULL;
			_state				= SS2_STEADY_STATE;
			break;
	}//end switch _state...

	/// Set the switched state to follow the new stream reference.
	if(_pNewStream == NULL)
		_switched = 1;
	else
		_switched = 0;

	*retBitLen = len;
	return(pRet);
}//end GetNextFrame.


/** Switch from the current stream to another stream.
A non-NULL stream in the pToStream parameter initiates the switching 
process. However, a switch can only be activated if a previous switch
is not in progress. A single call to this method is all that is required
and further calls will be rejected until the switch is complete.
@param pToStream		: Dual switchable stream to switch to.
@return							: 1 = success, 0 = switch request not accepted.
*/
int StreamSwitcher2::Switch(void* pToStream)
{
	if( (pToStream == NULL)||(_switched == 0) )
		return(0);

	_pNewStream = pToStream;
	return(1);
}//end Switch.


