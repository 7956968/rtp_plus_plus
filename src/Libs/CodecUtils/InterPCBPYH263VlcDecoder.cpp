/** @file

MODULE						: InterPCBPYH263VlcDecoder

TAG								: IPCBPYH263VD

FILE NAME					: InterPCBPYH263VlcDecoder.cpp

DESCRIPTION				: An CBPY Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.5
										page 37 and Table 13 with an IVlcEncoder Interface.

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

#include "InterPCBPYH263VlcDecoder.h"

InterPCBPYH263VlcDecoder::InterPCBPYH263VlcDecoder()
{
	_numCodeBits	= 0;
}//end constructor.

InterPCBPYH263VlcDecoder::~InterPCBPYH263VlcDecoder()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Decode a CBPY symbol from the bit stream.
Extract the symbols from the input stream. Decode as an inline binary 
tree to determine the symbol. An incorrect code can still be decoded.
@param bsr			: Bit stream to get from.
@return					: Decoded CBPY value.
*/
int	InterPCBPYH263VlcDecoder::Decode(IBitStreamReader* bsr)
{
	int cbpy	= 0;
	int bits	= 0;

	// Construct a binary tree with a "need for speed".
	if(bsr->Read())	// 1
	{
		if(bsr->Read())	// 11
		{
			bits = 2;
			cbpy = 0;
		}//end if 11...
		else						// 10
		{
			int next2 = bsr->Read(2);
			bits = 4;
			switch(next2)
			{
				case 0:					// 1000
					cbpy = 2;
					break;
				case 1:					// 1001
					cbpy = 12;
					break;
				case 2:					// 1010
					cbpy = 4;
					break;
				case 3:					// 1011
					cbpy = 8;
					break;
			}//end switch next2...
		}//end else 10...
	}//end if 1...
	else						// 0
	{
		if(bsr->Read())	// 01
		{
			int next2 = bsr->Read(2);
			bits = 4;
			switch(next2)
			{
				case 0:					// 0100
					cbpy = 3;
					break;
				case 1:					// 0101
					cbpy = 5;
					break;
				case 2:					// 0110
					cbpy = 1;
					break;
				case 3:					// 0111
					cbpy = 10;
					break;
			}//end switch next2...
		}//end if 01...
		else						// 00
		{
			if(bsr->Read())	// 001
			{
				if(bsr->Read())	// 0011
				{
					bits = 4;
					cbpy = 15;
				}//end if 0011...
				else						// 0010
				{
					if(bsr->Read())	// 0010 1
					{
						bits = 5;
						cbpy = 14;
					}//end if 0010 1...
					else						// 0010 0
					{
						bits = 5;
						cbpy = 13;
					}//end else 0010 0...
				}//end else 0010...
			}//end if 001...
			else						// 000
			{
				if(bsr->Read())	// 0001
				{
					if(bsr->Read())	// 0001 1
					{
						bits = 5;
						cbpy = 11;
					}//end if 0001 1...
					else						// 0001 0
					{
						bits = 5;
						cbpy = 7;
					}//end else 0001 0...
				}//end if 0001...
				else						// 0000
				{
					int next2 = bsr->Read(2);
					if(next2 == 2)	// 0000 10
					{
						bits = 6;
						cbpy = 9;
					}//end if 0000 10...
					else if(next2 == 3)	// 0000 11
					{
						bits = 6;
						cbpy = 6;
					}//end else if 0000 11...
				}//end else 0000...
			}//end else 000...
		}//end else 00...
	}//end else 0...

	_numCodeBits = bits;
  return(cbpy);
}//end Decode.

