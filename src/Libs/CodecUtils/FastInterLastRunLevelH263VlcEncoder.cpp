/** @file

MODULE						: FastInterLastRunLevelH263VlcEncoder

TAG								: FLRLH263VE

FILE NAME					: FastInterLastRunLevelH263VlcEncoder.cpp

DESCRIPTION				: A fast Inter last-run-level Vlc encoder implementation as defined in
										H.263 Recommendation (02/98). This is defined in the Inter and the std
										Intra mode and is implemented with an	IVlcEncoder Interface. It extends 
										from an LastRunLevelH263VlcEncoder() class to make a look up table of 
										some of the symbol combinations.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include "FastInterLastRunLevelH263VlcEncoder.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
FastInterLastRunLevelH263VlcEncoder::FastInterLastRunLevelH263VlcEncoder(void)
{
	/// Set the table values by encoding combinations using the base class. The 
	/// table address is constructed in the following way:
	/// 13 bits = 1 bit (last) + 6 bits (run) + 6 bits (level). [level range = -32..+31]
	for(int last = 0; last < 2; last++)
	{
		for(int run = 0; run < 64; run++)
		{
			for(int level = -32; level < 32; level++)
			{
				int index = (level & 0x3F) | ((run & 0x3F) << 6) | ((last & 0x01) << 12);
				/// Do the encode with the base class to get the codeword and its length.
				VLC_LENGTH[index]		= LastRunLevelH263VlcEncoder::Encode3(last, run, level);
				VLC_CODEWORD[index] = _bitCode;
			}//end for level...
		}//end for run...
	}//end for last...

}//end constructor.

FastInterLastRunLevelH263VlcEncoder::~FastInterLastRunLevelH263VlcEncoder(void)
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Encode last-run-level symbols.
The 3 param symbols represent last, run and level, respectively and are
encoded with the number of bits returned. The _numCodeBits and _bitCode
members are set accordingly. For fast access a look up table method is
used for a subset of the encodings. Escape sequence for the rest.
@param symbol1	: last
@param symbol2	: run
@param symbol3	: level
@return					: Num of bits in the codeword.
*/
int	FastInterLastRunLevelH263VlcEncoder::Encode3(int symbol1, int symbol2, int symbol3)
{
	int last	= symbol1;
	int run		= symbol2;
	int level = symbol3;

	/// The quantisation process is responsible for clipping the level.

	_bitCode			= 0;
	_numCodeBits	= 0;

	/// Use the look up table if the run and level symbols are in range.
	if( (run < 32)&&(level >= -32)&&(level < 32) )
	{
		int index = (level & 0x3F) | ((run & 0x3F) << 6) | ((last & 0x01) << 12);
		if(index < FLRLH263VE_TABLE_LENGTH)
		{
			_numCodeBits	= VLC_LENGTH[index];
			_bitCode			= VLC_CODEWORD[index];
		}//end if index...
		else
			return(_numCodeBits);	///< Failure.
	}//end if run...
	else
	{
		EncodeESC(last, run, level);
	}//end else...

	return(_numCodeBits);
}// end Encode3.


/*
---------------------------------------------------------------------------
	Private Methods.
---------------------------------------------------------------------------
*/


