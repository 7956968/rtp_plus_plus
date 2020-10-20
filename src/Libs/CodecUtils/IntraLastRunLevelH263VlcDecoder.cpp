/** @file

MODULE						: IntraLastRunLevelH263VlcDecoder

TAG								: ILRLH263VD

FILE NAME					: IntraLastRunLevelH263VlcDecoder.cpp

DESCRIPTION				: An Intra last-run-level Vlc decoder implementation as defined in
										H.263 Recommendation (02/98) Annex D Table I.2 page 75. This is
										defined as the Advanced Intra mode and is implemented with an
										IVlcDecoder Interface. The table has the same codes as those used
										for the AC coeff but with a different interpretation for the run,
										level values.

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

#include "IntraLastRunLevelH263VlcDecoder.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
#define ILRLH263VD_NUM_ESC_CODE_BITS			7			// ESC marker code.
#define ILRLH263VD_ESC_CODEWORD			 0x0003
#define ILRLH263VD_ESC_TABLE_POS				 26

IntraLastRunLevelH263VlcDecoder::IntraLastRunLevelH263VlcDecoder()
{
	_numCodeBits	= 0;
}//end constructor.

IntraLastRunLevelH263VlcDecoder::~IntraLastRunLevelH263VlcDecoder()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Decode a last-run-level symbol from the bit stream.
Extract the symbol triplet from the input stream. Decode as a binary tree 
to determine the last-run-level. The table must be in ascending bit length 
order. Bits are extracted from the bit stream depending on the next no. of 
bits in the table. Keep going through the table until a match is found or 
the table ends.
@param bsr			: Bit stream to get from.
@param symbol1	: Returned last value.
@param symbol2	: Returned run value.
@param symbol3	: Returned signed level value.
@return					: Num bits extracted.
*/
int	IntraLastRunLevelH263VlcDecoder::Decode3(IBitStreamReader* bsr, int* symbol1, int* symbol2, int* symbol3)
{
  int bit;
	int last				= 0;
	int	run					= 0;
	int level				= 0;
  int bits				= 0;
  int bitsSoFar		= 0;
	int bitsNeeded	= 0;

  // Keep going through the table until a match is found 
	// or the table ends.
  int tblPos			= 0;
	int tblSize			= ILRLH263VD_TABLE_LENGTH; // Include markers.
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
				if(tblPos != ILRLH263VD_ESC_TABLE_POS)
				{
					last	= VLC_TABLE[tblPos].last;
					run		= VLC_TABLE[tblPos].run;
					level = VLC_TABLE[tblPos].level;
					// Get the sign bit.
					bit = bsr->Read();
					bitsSoFar++;
					if(bit)
						level = -level;
				}//end if !ESC
				else	// ESC sequence.
				{
					last	= bsr->Read();	// Last = 1 bit.
					run		= bsr->Read(6);	// Run = 6 bits.
					level = (int)((char)(0xFF & bsr->Read(8)));	// Level with sign = 8 bits.
					bitsSoFar += 15;
				}//end else...
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
		last					= 2;
		run						= 0;
		level					= 0;
    _numCodeBits	= 0; // Implies an error. 
  }//end if !found...

	// Set symbols.
	*symbol1 = last;
	*symbol2 = run;
	*symbol3 = level;
  return(_numCodeBits);
}//end Decode3.

/*
---------------------------------------------------------------------------
	Constant vlc table.
---------------------------------------------------------------------------
*/
const LastRunLevelType IntraLastRunLevelH263VlcDecoder::VLC_TABLE[ILRLH263VD_TABLE_LENGTH] =
{//last run level bits code
	{	0,  0,	1,	2,	0x0002 },	//  0
	{	0,	0,	2,	3,	0x0006 },	//  1
	{	0,	0,	3,	4,	0x000E },	//  2
	{	0,	1,	1,	4,	0x000F },	//  3
	{	1,	0,	1,	4,	0x0007 },	//  4
	{	0,	0,	5,	5,  0x000D },	//  5
	{	0,	0,	4,	5,	0x000C },	//  6
	{	0,	2,	1,	5,	0x000B },	//  7
	{	0,	0,	8,	6,	0x0012 },	//  8
	{	0,	0,	7,	6,	0x0011 },	//  9
	{	0,	0,	6,	6,	0x0010 },	// 10
	{	0,	1,	2,	6,	0x0014 },	// 11
	{	0,	3,	1,	6,	0x0015 },	// 12
	{	0,	4,	1,	6,	0x0013 },	// 13	
	{	1,	0,	2,	6,	0x000C },	// 14	
	{	1,	1,	1,	6,	0x000F },	// 15	
	{	1,	2,	1,	6,	0x000E },	// 16	
	{	1,	3,	1,	6,	0x000D },	// 17	
	{	0,	0,	9,	7,	0x0016 },	// 18	
	{	0,	1,	3,	7,	0x0014 },	// 19	
	{	0,	2,	2,	7,	0x0015 },	// 20	
	{	0,	5,	1,	7,	0x0017 },	// 21	
	{	1,	0,	3,	7,	0x0010 },	// 22	
	{	1,	4,	1,	7,	0x0011 },	// 23	
	{	1,	5,	1,	7,	0x0013 },	// 24	
	{	1,	6,	1,	7,	0x0012 },	// 25	
	{	2,	0,	0,	7,	0x0003 },	// 26	ESC Marker
	{	0,	0, 10,	8,	0x001B },	// 27	
	{	0,	1,	4,	8,	0x001E },	// 28	
	{	0,	3,	2,	8,	0x001D },	// 29
	{	0,	6,	1,	8,	0x001C },	// 30
	{	0,	7,	1,	8,	0x001F },	// 31
	{	1,	0,	4,	8,	0x0013 },	// 32
	{	1,	7,	1,	8,	0x0014 },	// 33
	{	1,	8,	1,	8,	0x0015 },	// 34
	{	1,	9,	1,	8,	0x001A },	// 35
	{	1, 10,	1,	8,	0x0019 },	// 36
	{	1, 11,	1,	8,	0x0018 },	// 37
	{	1, 12,	1,	8,	0x0017 },	// 38
	{	1, 13,	1,	8,	0x0016 },	// 39
	{	0,  0, 12,	9,	0x0021 },	// 40
	{	0,  0, 11,	9,	0x0020 },	// 41
	{	0,	0, 18,	9,	0x001F },	// 42
	{	0,	0, 17,	9,	0x001E },	// 43
	{	0,	0, 16,	9,	0x001D },	// 44
	{	0,	0, 15,	9,	0x001C },	// 45
	{	0,	0, 14,	9,	0x001B },	// 46
	{	0,	0, 13,	9,	0x001A },	// 47
	{	0,	4,	2,	9,	0x0023 },	// 48
	{	0,	5,	2,	9,	0x0022 },	// 49
	{	0,	8,	1,	9,	0x0025 },	// 50
	{	0,	9,	1,	9,	0x0024 },	// 51
	{	1,	0,	6,	9,	0x0012 },	// 52
	{	1,	0,	5,	9,	0x0011 },	// 53
	{	1,	1,	2,	9,	0x0013 },	// 54
	{	1,	2,	2,	9,	0x0014 },	// 55
	{	1, 14,	1,	9,	0x0019 },	// 56
	{	1, 15,	1,	9,	0x0015 },	// 57
	{	1, 16,	1,	9,	0x0016 },	// 58
	{	1, 17,	1,	9,	0x0018 },	// 59
	{	1, 18,	1,	9,	0x0017 },	// 60
	{	0,	1,	5, 10,	0x000F },	// 61
	{	0,	2,	3, 10,	0x000E },	// 62
	{	0,	2,	4, 10,	0x0009 },	// 63
	{	0,	3,	3, 10,	0x000D },	// 64
	{	0,	6,	2, 10,	0x000C },	// 65
	{	0,	7,	2, 10,	0x000B },	// 66
	{	0,	8,	2, 10,	0x000A },	// 67
	{	0, 10,	1, 10,	0x0021 },	// 68
	{	0, 11,	1, 10,	0x0020 },	// 69
	{	0, 12,	1, 10,	0x0008 },	// 70
	{	1,	0,	7, 10,	0x0004 },	// 71
	{	1,	1,  2, 10,	0x0005 },	// 72
	{	1,	3,	2, 10,	0x0006 },	// 73
	{	1,	4,	2, 10,	0x0007 },	// 74
	{	0,	0, 20, 11,	0x0022 },	// 75
	{	0,	0, 19, 11,	0x0023 },	// 76
	{	0,	1,	6, 11,	0x0021 },	// 77
	{	0,	4,	3, 11,	0x0007 },	// 78
	{	0,	9,	2, 11,	0x0006 },	// 79
	{	0, 13,	1, 11,	0x0020 },	// 80
	{	1,	0,	9, 11,	0x0026 },	// 81
	{	1,	0,	8, 11,	0x0027 },	// 82
	{	1,	1,	4, 11,	0x0025 },	// 83
	{	1,	2,	3, 11,	0x0024 },	// 84
	{	1, 19,	1, 11,	0x0004 },	// 85
	{	1, 20,	1, 11,	0x0005 },	// 86
	{	0,	0, 25, 12,	0x0053 },	// 87
	{	0,	0, 24, 12,	0x0054 },	// 88
	{	0,	0, 23, 12,	0x0055 },	// 89
	{	0,	0, 22, 12,	0x0056 },	// 90
	{	0,	0, 21, 12,	0x0057 },	// 91
	{	0,	1,	7, 12,	0x0050 },	// 92
	{	0,	3,	4, 12,	0x0051 },	// 93
	{	0,	5,	3, 12,	0x0052 },	// 94
	{	1,	0, 10, 12,	0x005F },	// 95
	{	1,	3,	3, 12,	0x005E },	// 96
	{	1,	5,	2, 12,	0x005D },	// 97
	{	1,	6,	2, 12,	0x005C },	// 98
	{	1,	7,	2, 12,	0x005B },	// 99
	{	1, 21,	1, 12,	0x0058 },	// 100
	{	1, 22,	1, 12,	0x0059 },	// 101
	{	1, 23,	1, 12,	0x005A }	// 102
};

