/** @file

MODULE						: MotionVectorH263VlcDecoderImplStd

TAG								: MVH263VDIS

FILE NAME					: MotionVectorH263VlcDecoderImplStd.cpp

DESCRIPTION				: The base level H.263 motion vector Vlc decoder implementation 
										with an	IVlcDecoder Interface. As defined by Recommendation 
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

#include "MotionVectorH263VlcDecoderImplStd.h"

/*
---------------------------------------------------------------------------
	Constructor and Destructor.
---------------------------------------------------------------------------
*/

MotionVectorH263VlcDecoderImplStd::MotionVectorH263VlcDecoderImplStd()
{
	_numCodeBits	= 0;
}//end constructor.

MotionVectorH263VlcDecoderImplStd::~MotionVectorH263VlcDecoderImplStd()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Decode a dual possible motion vector coord symbol from the bit stream.
Extract the dual symbol from the input stream. Decode as a binary tree 
to determine the vector coord. The table must be in ascending bit length 
order. Bits are extracted from the bit stream depending on the next no. of 
bits in the table. Keep going through the table until a match is found or 
the table ends.
@param bsr			: Bit stream to get from.
@param symbol1	: Returned 1st possible value.
@param symbol2	: Returned 2nd possible value.
@return					: Num bits extracted.
*/
int	MotionVectorH263VlcDecoderImplStd::Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2)
{
  int bit;
	int poss1				= 0;
	int	poss2				= 0;
  int bits				= 0;
  int bitsSoFar		= 0;
	int bitsNeeded	= 0;

  // Keep going through the table until a match is found 
	// or the table ends.
  int tblPos			= 0;
	int tblSize			= MVH263VDIS_TABLE_LENGTH;
  int found				= 0;
  while( (tblPos < tblSize) && !found )
  {
    bitsNeeded = VLC_TABLE[tblPos].numBits - bitsSoFar;
    // Get the bits off the bit stream.
		bit  = bsr->Read(bitsNeeded);
		bits = (bits << bitsNeeded) | bit;
    bitsSoFar += bitsNeeded;

    // Advance down the table checking the codes with the current 
		// no. of bits so far.
    while( (VLC_TABLE[tblPos].numBits == bitsSoFar) && 
           (tblPos < tblSize) && !found )
    {
      if(VLC_TABLE[tblPos].codeWord == bits)
			{
        found = 1;
				poss1	= VLC_TABLE[tblPos].possibleCoord1;
				poss2 = VLC_TABLE[tblPos].possibleCoord2;
			}//end if codeWord...
      else
        tblPos++;

    }//end while VLC_TABLE...

  }//end while tblPos...
	// Set bits extracted.
  _numCodeBits = bitsSoFar;

  // If not found then there is an error.
  if( !found )
  {
		poss1					= 0;
		poss2					= 0;
    _numCodeBits	= 0; // Implies an error. 
  }//end if !found...

	// Set symbols.
	*symbol1 = poss1;
	*symbol2 = poss2;
  return(_numCodeBits);
}//end Decode2.

/*
---------------------------------------------------------------------------
	Constant vlc table.
---------------------------------------------------------------------------
*/
const DualMotionVectorType MotionVectorH263VlcDecoderImplStd::VLC_TABLE[MVH263VDIS_TABLE_LENGTH] =
{//vec1   vec2  bits  code
	{	  0,		0,	 1,	0x0001 },	//	 0
	{	  1,	-63,	 3,	0x0002 },	//	 1
	{	 -1,	 63,	 3,	0x0003 },	//	 2
	{	  2,	-62,	 4,	0x0002 },	//	 3
	{	 -2,	 62,	 4,	0x0003 },	//	 4
	{	  3,	-61,	 5,	0x0002 },	//	 5
	{	 -3,	 61,	 5,	0x0003 },	//	 6
	{	  4,	-60,	 7,	0x0006 },	//	 7
	{	 -4,	 60,	 7,	0x0007 },	//	 8
	{	  5,	-59,	 8,	0x000A },	//	 9
	{	 -5,	 59,	 8,	0x000B },	//	10
	{	  6,	-58,	 8,	0x0008 },	//	11
	{	 -6,	 58,	 8,	0x0009 },	//	12
	{	  7,	-57,	 8,	0x0006 },	//	13
	{	 -7,	 57,	 8,	0x0007 },	//	14
	{	  8,	-56,	10,	0x0016 },	//	15
	{	 -8,	 56,	10,	0x0017 },	//	16
	{	  9,	-55,	10,	0x0014 },	//	17
	{	 -9,	 55,	10,	0x0015 },	//	18
	{	 10,	-54,	10,	0x0012 },	//	19
	{	-10,	 54,	10,	0x0013 },	//	20
	{	 11,	-53,	11,	0x0022 },	//	21
	{	-11,	 53,	11,	0x0023 },	//	22
	{	 12,	-52,	11,	0x0020 },	//	23
	{	-12,	 52,	11,	0x0021 },	//	24
	{	 13,	-51,	11,	0x001E },	//	25
	{	-13,	 51,	11,	0x001F },	//	26
	{	 14,	-50,	11,	0x001C },	//	27
	{	-14,	 50,	11,	0x001D },	//	28
	{	 15,	-49,	11,	0x001A },	//	29
	{	-15,	 49,	11,	0x001B },	//	30
	{	 16,	-48,	11,	0x0018 },	//	31
	{	-16,	 48,	11,	0x0019 },	//	32
	{	 17,	-47,	11,	0x0016 },	//	33
	{	-17,	 47,	11,	0x0017 },	//	34
	{	 18,	-46,	11,	0x0014 },	//	35
	{	-18,	 46,	11,	0x0015 },	//	36
	{	 19,	-45,	11,	0x0012 },	//	37
	{	-19,	 45,	11,	0x0013 },	//	38
	{	 20,	-44,	11,	0x0010 },	//	39
	{	-20,	 44,	11,	0x0011 },	//	40
	{	 21,	-43,	11,	0x000E },	//	41
	{	-21,	 43,	11,	0x000F },	//	42
	{	 22,	-42,	11,	0x000C },	//	43
	{	-22,	 42,	11,	0x000D },	//	44
	{	 23,	-41,	11,	0x000A },	//	45
	{	-23,	 41,	11,	0x000B },	//	46
	{	 24,	-40,	11,	0x0008 },	//	47
	{	-24,	 40,	11,	0x0009 },	//	48
	{	 25,	-39,	12,	0x000E },	//	49
	{	-25,	 39,	12,	0x000F },	//	50
	{	 26,	-38,	12,	0x000C },	//	51
	{	-26,	 38,	12,	0x000D },	//	52
	{	 27,	-37,	12,	0x000A },	//	53
	{	-27,	 37,	12,	0x000B },	//	54
	{	 28,	-36,	12,	0x0008 },	//	55
	{	-28,	 36,	12,	0x0009 },	//	56
	{	 29,	-35,	12,	0x0006 },	//	57
	{	-29,	 35,	12,	0x0007 },	//	58
	{	 30,	-34,	12,	0x0004 },	//	59
	{	-30,	 34,	12,	0x0005 },	//	60
	{	 31,	-33,	13,	0x0006 },	//	61
	{	-31,	 33,	13,	0x0007 },	//	62
	{	-32,	 32,	13,	0x0005 }	//	63
};

