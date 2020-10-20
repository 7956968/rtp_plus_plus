/** @file

MODULE						: FastIntraLastRunLevelH263VlcEncoder

TAG								: FILRLH263VE

FILE NAME					: FastIntraLastRunLevelH263VlcEncoder.cpp

DESCRIPTION				: A fast Intra last-run-level Vlc encoder implementation as defined in
										H.263 Recommendation (02/98) Annex D Table I.2 page 75. This is
										defined in the Advanced Intra mode and is implemented with an
										IVlcEncoder Interface. It extends from an IntraLastRunLevelH263VlcEncoder()
										class to make a look up table of some of the symbol combinations.

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

#include "FastIntraLastRunLevelH263VlcEncoder.h"

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
FastIntraLastRunLevelH263VlcEncoder::FastIntraLastRunLevelH263VlcEncoder(void)
{
	/// Set the table values by encoding combinations using the base class. The 
	/// table address is constructed in the following way:
	/// 12 bits = 1 bit (last) + 5 bits (run) + 6 bits (level). [level range = -32..+31]
	for(int last = 0; last < 2; last++)
	{
		for(int run = 0; run < 32; run++)
		{
			for(int level = -32; level < 32; level++)
			{
				int index = (level & 0x3F) | ((run & 0x1F) << 6) | ((last & 0x01) << 11);
				/// Do the encode with the base class to get the codeword and its length.
				VLC_LENGTH[index]		= IntraLastRunLevelH263VlcEncoder::Encode3(last, run, level);
				VLC_CODEWORD[index] = _bitCode;
			}//end for level...
		}//end for run...
	}//end for last...

}//end constructor.

FastIntraLastRunLevelH263VlcEncoder::~FastIntraLastRunLevelH263VlcEncoder(void)
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
int	FastIntraLastRunLevelH263VlcEncoder::Encode3(int symbol1, int symbol2, int symbol3)
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
		int index = (level & 0x3F) | ((run & 0x1F) << 6) | ((last & 0x01) << 11);
		if(index < FILRLH263VE_TABLE_LENGTH)
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

/*
	int last	= symbol1;
	int run		= symbol2;
	int level = symbol3;
	int sign	= 0;
	if(symbol3 < 0)
	{
		sign = 1;
		level = -level;
	}//end if symbol3...

	// The quantisation process is responsible for clipping the level.

	_bitCode			= 0;
	_numCodeBits	= 0;

	// Jump into the table and set the search limit index depending
	// on the last param. Last = 1 is second part of table and does
	// not need to be tested in the loop.
	int tablePos		= 0;
	int tableLimit	= ILRLH263VE_LAST_JUMP;
	if(last)
	{
		tablePos		= ILRLH263VE_LAST_JUMP;
		tableLimit	= ILRLH263VE_TABLE_LENGTH;
	}//end if last...

	int found = 0;
	for(int i = tablePos; (i < tableLimit)&&(!found); i++)
	{
		if( (VLC_TABLE[i].run == run)&&(VLC_TABLE[i].level == level) )
		{
			// Tack the sign bit on to the LSB position.
			_numCodeBits	= VLC_TABLE[i].numBits + 1;
			_bitCode			= (VLC_TABLE[i].codeWord << 1) | sign;
			found = 1;
		}//end if found...
	}//end for i...

	// ESC sequence required.
	if(!found)
	{
		_bitCode = (ILRLH263VE_ESC_CODEWORD << 15);
		_bitCode |= (last << 14);
		_bitCode |= ((run & 0x3F) << 8);
		_bitCode |= (symbol3 & 0xFF);	// Regain the sign of level.
		_numCodeBits = 22;
	}//end if !found...
*/

	return(_numCodeBits);
}// end Encode3.


/*
---------------------------------------------------------------------------
	Private Methods.
---------------------------------------------------------------------------
*/


