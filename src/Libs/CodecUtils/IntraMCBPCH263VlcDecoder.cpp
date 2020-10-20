/** @file

MODULE						: IntraMCBPCH263VlcDecoder

TAG								: IMCBPCH263VD

FILE NAME					: IntraMCBPCH263VlcDecoder.cpp

DESCRIPTION				: An MCBPC Vlc decoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.2
										page 34 with an	IVlcDecoder Interface.

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

#include "IntraMCBPCH263VlcDecoder.h"

IntraMCBPCH263VlcDecoder::IntraMCBPCH263VlcDecoder()
{
	_numCodeBits	= 0;
	_marker				= 0;
}//end constructor.

IntraMCBPCH263VlcDecoder::~IntraMCBPCH263VlcDecoder()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Decode a MB type and CBPC symbol from the bit stream.
Extract the symbols from the input stream. Decode as a binary tree 
to determine the symbol pair.
@param bsr			: Bit stream to get from.
@param symbol1	: Returned MB type value.
@param symbol2	: Returned CBPC value.
@return					: Num bits extracted.
*/
int	IntraMCBPCH263VlcDecoder::Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2)
{
	int mbt		= 0;
	int cbpc	= 0;
	int bits	= 0;
	_marker		= 0;

	// Construct an inline binary tree with a "need for speed".
	if(bsr->Read())	// 1
	{
		bits	= 1;
		mbt		= 3;
		cbpc	= 0;
	}//end if 1...
	else						// 0
	{
		if(bsr->Read())	// 01
		{
			if(bsr->Read())	// 011
			{
				bits	= 3;
				mbt		= 3;
				cbpc	= 3;
			}//end if 011...
			else						// 010
			{
				bits	= 3;
				mbt		= 3;
				cbpc	= 2;
			}//end else 010...
		}//end if 01...
		else						// 00
		{
			if(bsr->Read())	// 001
			{
				bits	= 3;
				mbt		= 3;
				cbpc	= 1;
			}//end if 001...
			else						// 000
			{
				if(bsr->Read())	// 0001
				{
					bits	= 4;
					mbt		= 4;
					cbpc	= 0;
				}//end if 0001...
				else						// 0000
				{
					if(bsr->Read())	// 0000 1
					{
						if(bsr->Read())	// 0000 11
						{
							bits	= 6;
							mbt		= 4;
							cbpc	= 3;
						}//end if 0000 11...
						else						// 0000 10
						{
							bits	= 6;
							mbt		= 4;
							cbpc	= 2;
						}//end else 0000 10...
					}//end if 0000 1...
					else						// 0000 0
					{
						if(bsr->Read())	// 0000 01
						{
							bits	= 6;
							mbt		= 4;
							cbpc	= 1;
						}//end if 0000 01...
						else						// 0000 00
						{
							int stuffing = bsr->Read(3);
							if(stuffing == 1)	// 0000 0000 1
							{
								bits	= 9;
								mbt		= 3;	// Meaningless for marker.
								cbpc	= 3;
								_marker = 1;
							}//end if stuffing 0000 0000 1...
						}//end else 0000 00...
					}//end else 0000 0...
				}//end else 0000...
			}//end else 000...
		}//end else 00...
	}//end else 0...

	// Set symbols.
	*symbol1			= mbt;
	*symbol2			= cbpc;
	_numCodeBits	= bits;
  return(bits);
}//end Decode2.

