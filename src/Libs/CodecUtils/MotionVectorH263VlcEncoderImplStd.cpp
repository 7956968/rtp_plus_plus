/** @file

MODULE						: MotionVectorH263VlcEncoderImplStd

TAG								: MVH263VEIS

FILE NAME					: MotionVectorH263VlcEncoderImplStd.cpp

DESCRIPTION				: The base level H.263 motion vector Vlc encoder implementation 
										with an	IVlcEncoder Interface. As defined by Recommendation 
										H.263 (02/98) Table 14 page 39 and section 5.3.7.

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

#include "MotionVectorH263VlcEncoderImplStd.h"

/*
---------------------------------------------------------------------------
	Construction and Destruction.
---------------------------------------------------------------------------
*/
MotionVectorH263VlcEncoderImplStd::MotionVectorH263VlcEncoderImplStd(void)
{
	_numCodeBits	= 0;
	_bitCode			= 0;
}//end construction.

MotionVectorH263VlcEncoderImplStd::~MotionVectorH263VlcEncoderImplStd(void)
{
}//end destruction.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Encode the param pair as an (x,y) vector.
Treat the symbols as independent single encodes and concatinate
them in the correct order of the _bitCode.
@param symbol1	: X coord of the motion vector.
@param symbol2	: Y coord of the motion vector.
@return					: Total num of coded bits.
*/
int MotionVectorH263VlcEncoderImplStd::Encode2(int symbol1,int symbol2)
{
  int xbits,ybits;
  int xcode = 0;
  int ycode = 0;

  xbits = Encode(symbol1);
	xcode = _bitCode;
  if(!xbits)
    return(0);

  ybits = Encode(symbol2);
  if(!ybits)
    return(0);

	// X in MSB part of codeword.
	_bitCode = _bitCode | (xcode << ybits);
	_numCodeBits = xbits + ybits;

  return(_numCodeBits);
}//end Encode2.

/** Encode the symbol as a motion vector coord.
The symbol is in half pel units. i.e. 3 = 1.5 There
are 2 possible symbols that match 1 vlc table index.
@param symbol	: Coord of the motion vector.
@return				: Num of coded bits.
*/
int MotionVectorH263VlcEncoderImplStd::Encode(int symbol)
{
	_numCodeBits	= 0;
	_bitCode			= 0;
	// Range check in half pel units.
	if( (symbol < -63)||(symbol > 63) )
		return(0);

	int coord = symbol;
	// Translate the 2 possible symbols to 1 coord in the
	// range [-32..31].
	if(coord > 31)
		coord = coord - 64;
	else if(coord < -32)
		coord = coord + 64;

	_numCodeBits	= VLC_TABLE[coord + 32][MVH263VEIS_NUM_BITS];
	_bitCode			= VLC_TABLE[coord + 32][MVH263VEIS_BIT_CODE];
	return(_numCodeBits);
}//end Encode.

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
const int MotionVectorH263VlcEncoderImplStd::VLC_TABLE[MVH263VEIS_TABLE_SIZE][2] =
{	// Num of bits, Code word					index
	{ 13 					,	0x0005			 },		//	0   
	{ 13 					, 0x0007			 },		//	1
	{ 12 					, 0x0005			 },  	//  2
	{ 12 					, 0x0007			 },		//	3
	{ 12 					, 0x0009			 },  	//  4
	{ 12 					, 0x000B			 },		//	5
	{ 12 					, 0x000D			 },  	//  6
	{ 12 					, 0x000F			 },		//	7
	{ 11 					, 0x0009			 },		//	8
	{ 11 					, 0x000B			 },		//	9
	{ 11 					, 0x000D			 },		// 10 
	{ 11 					, 0x000F			 },		// 11
	{ 11 					, 0x0011			 },		// 12
	{ 11 					, 0x0013			 },		// 13
	{ 11 					, 0x0015			 },		// 14
	{ 11 					, 0x0017			 },		// 15
	{ 11 					, 0x0019			 },		// 16
	{ 11 					, 0x001B			 },		// 17
	{ 11 					, 0x001D			 },		// 18
	{ 11 					, 0x001F			 },		// 19
	{ 11 					, 0x0021			 },		// 20
	{ 11 					, 0x0023			 },		// 21
	{ 10 					, 0x0013			 },		// 22
	{ 10 					, 0x0015			 },		// 23
	{ 10 					, 0x0017			 },		// 24
	{ 8 					, 0x0007			 },		// 25
	{ 8 					, 0x0009			 },		// 26
	{ 8 					, 0x000B			 },		// 27
	{ 7 					, 0x0007			 },		// 28
	{ 5 					, 0x0003			 },		// 29
	{ 4 					, 0x0003			 },		// 30
	{ 3 					, 0x0003			 },		// 31
	{ 1 					, 0x0001			 },		// 32
	{ 3 					, 0x0002			 },		// 33
	{ 4 					, 0x0002			 },		// 34
	{ 5 					, 0x0002			 },		// 35
	{ 7 					, 0x0006			 },		// 36
	{ 8 					, 0x000A			 },		// 37
	{ 8 					, 0x0008			 },		// 38
	{ 8 					, 0x0006			 },		// 39
	{ 10 					, 0x0016			 },		// 40
	{ 10 					, 0x0014			 },		// 41
	{ 10 					, 0x0012			 },		// 42
	{ 11 					, 0x0022			 },		// 43
	{ 11 					, 0x0020			 },		// 44
	{ 11 					, 0x001E			 },		// 45
	{ 11 					, 0x001C			 },		// 46
	{ 11 					, 0x001A			 },		// 47
	{ 11 					, 0x0018			 },		// 48
	{ 11 					, 0x0016			 },		// 49
	{ 11 					, 0x0014			 },		// 50
	{ 11 					, 0x0012			 },		// 51
	{ 11 					, 0x0010			 },		// 52
	{ 11 					, 0x000E			 },		// 53
	{ 11 					, 0x000C			 },		// 54
	{ 11 					, 0x000A			 },		// 55
	{ 11 					, 0x0008			 },		// 56
	{ 12 					, 0x000E			 },		// 57
	{ 12 					, 0x000C			 },		// 58
	{ 12 					, 0x000A			 },		// 59
	{ 12 					, 0x0008			 },		// 60
	{ 12 					, 0x0006			 },		// 61
	{ 12 					, 0x0004			 },		// 62
	{ 13 					, 0x0006			 }		// 63
};

