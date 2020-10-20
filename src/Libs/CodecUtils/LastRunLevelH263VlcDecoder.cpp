/** @file

MODULE						: LastRunLevelH263VlcDecoder

TAG								: LRLH263VD

FILE NAME					: LastRunLevelH263VlcDecoder.cpp

DESCRIPTION				: A last-run-level Vlc decoder implementation as defined 
										in the H.263 Recommendation (02/98) with an	IVlcDecoder 
										Interface.

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

#include "LastRunLevelH263VlcDecoder.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
#define LRLH263VD_NUM_ESC_CODE_BITS				7			// ESC marker code.
#define LRLH263VD_ESC_CODEWORD			 0x0003
#define LRLH263VD_ESC_TABLE_POS					 26

LastRunLevelH263VlcDecoder::LastRunLevelH263VlcDecoder()
{
	_numCodeBits	= 0;
}//end constructor.

LastRunLevelH263VlcDecoder::~LastRunLevelH263VlcDecoder()
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
int	LastRunLevelH263VlcDecoder::Decode3(IBitStreamReader* bsr, int* symbol1, int* symbol2, int* symbol3)
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
	int tblSize			= LRLH263VD_TABLE_LENGTH; // Include markers.
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
				if(tblPos != LRLH263VD_ESC_TABLE_POS)
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

/** Decode a last-run-level symbol from a codeword.
Extract the symbol triplet from the table using the input codeword and its
bit length. The table must be in ascending bit length order. Keep going 
through the table until a match is found or the table ends. ESC symbols
are dealt with as a special case.
@param numBits	: Bits in the codeword.
@param codeword	: Codeword to decode.
@param symbol1	: Returned last value.
@param symbol2	: Returned run value.
@param symbol3	: Returned signed level value.
@return					: Num bits extracted.
*/
int	LastRunLevelH263VlcDecoder::Decode3(int numBits, int codeword, int* symbol1, int* symbol2, int* symbol3)
{
  _numCodeBits	= numBits; // Assume all is well until proven otherwise.
	int last			= 0;
	int	run				= 0;
	int level			= 0;
	int found			= 0;

	// Treat an ESC sequence as a special case. ESC codes are 22 bits in length.
	if(numBits != 22)
	{
		// Establish and strip the sign bit off. The table excludes the sign.
		int sign = codeword & 1; // 0 = positive, 1 = negative.
		codeword = codeword >> 1;
		numBits--;

		// Match the number of bits before checking against the codeword.
		int tblPos	= 0;
		while( (tblPos < LRLH263VD_TABLE_LENGTH) && !found )
		{
			if(VLC_TABLE[tblPos].numBits == numBits)
			{
				if(VLC_TABLE[tblPos].codeWord == codeword)
				{
					found = 1;
					last	= VLC_TABLE[tblPos].last;
					run		= VLC_TABLE[tblPos].run;
					level = VLC_TABLE[tblPos].level;
					if(sign)
						level = -level;
				}//end if codeword...
			}//end if numBits...

			tblPos++;
		}//end while tblPos...

	}//end if numBits (non-ESC)...
	else
	{
		found			= 1;
		level			= (int)((char)(0xFF & codeword));	// Level with sign = 8 bits.
		codeword	= codeword >> 8;
		run				= 0x3F & codeword;	// Run = 6 bits.
		codeword	= codeword >> 6;
		last			= codeword & 1;
	}//end else ESC...

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
const LastRunLevelType LastRunLevelH263VlcDecoder::VLC_TABLE[LRLH263VD_TABLE_LENGTH] =
{//last run level bits code
	{	 0,	 0,	 1,	 2,	0x0002 },	//	 0
	{	 0,	 1,	 1,	 3,	0x0006 },	//	 1
	{	 0,	 0,	 2,	 4,	0x000F },	//	 2
	{	 0,	 2,	 1,	 4,	0x000E },	//	 3
	{	 1,	 0,	 1,	 4,	0x0007 },	//	 4
	{	 0,	 3,	 1,	 5,	0x000D },	//	 5
	{	 0,	 4,	 1,	 5,	0x000C },	//	 6
	{	 0,	 5,	 1,	 5,	0x000B },	//	 7
	{	 0,	 0,	 3,	 6,	0x0015 },	//	 8
	{	 0,	 1,	 2,	 6,	0x0014 },	//	 9
	{	 0,	 6,	 1,	 6,	0x0013 },	//	10
	{	 0,	 7,	 1,	 6,	0x0012 },	//	11
	{	 0,	 8,	 1,	 6,	0x0011 },	//	12
	{	 0,	 9,	 1,	 6,	0x0010 },	//	13
	{	 1,	 1,	 1,	 6,	0x000F },	//	14
	{	 1,	 2,	 1,	 6,	0x000E },	//	15
	{	 1,	 3,	 1,	 6,	0x000D },	//	16
	{	 1,	 4,	 1,	 6,	0x000C },	//	17
	{	 0,	 0,	 4,	 7,	0x0017 },	//	18
	{	 0,	10,	 1,	 7,	0x0016 },	//	19
	{	 0,	11,	 1,	 7,	0x0015 },	//	20
	{	 0,	12,	 1,	 7,	0x0014 },	//	21
	{	 1,	 5,	 1,	 7,	0x0013 },	//	22
	{	 1,	 6,	 1,	 7,	0x0012 },	//	23
	{	 1,	 7,	 1,	 7,	0x0011 },	//	24
	{	 1,	 8,	 1,	 7,	0x0010 },	//	25
	{	 2,	 0,	 0,	 7,	0x0003 },	//	26	ESC marker.
	{	 0,	 0,	 5,	 8,	0x001F },	//	27
	{	 0,	 1,	 3,	 8,	0x001E },	//	28
	{	 0,	 2,	 2,	 8,	0x001D },	//	29
	{	 0,	13,	 1,	 8,	0x001C },	//	30
	{	 0,	14,	 1,	 8,	0x001B },	//	31
	{	 1,	 9,	 1,	 8,	0x001A },	//	32
	{	 1,	10,	 1,	 8,	0x0019 },	//	33
	{	 1,	11,	 1,	 8,	0x0018 },	//	34
	{	 1,	12,	 1,	 8,	0x0017 },	//	35
	{	 1,	13,	 1,	 8,	0x0016 },	//	36
	{	 1,	14,	 1,	 8,	0x0015 },	//	37
	{	 1,	15,	 1,	 8,	0x0014 },	//	38
	{	 1,	16,	 1,	 8,	0x0013 },	//	39
	{	 0,	 0,	 6,	 9,	0x0025 },	//	40
	{	 0,	 0,	 7,	 9,	0x0024 },	//	41
	{	 0,	 3,	 2,	 9,	0x0023 },	//	42
	{	 0,	 4,	 2,	 9,	0x0022 },	//	43
	{	 0,	15,	 1,	 9,	0x0021 },	//	44
	{	 0,	16,	 1,	 9,	0x0020 },	//	45
	{	 0,	17,	 1,	 9,	0x001F },	//	46
	{	 0,	18,	 1,	 9,	0x001E },	//	47
	{	 0,	19,	 1,	 9,	0x001D },	//	48
	{	 0,	20,	 1,	 9,	0x001C },	//	49
	{	 0,	21,	 1,	 9,	0x001B },	//	50
	{	 0,	22,	 1,	 9,	0x001A },	//	51
	{	 1,	 0,	 2,	 9,	0x0019 },	//	52
	{	 1,	17,	 1,	 9,	0x0018 },	//	53
	{	 1,	18,	 1,	 9,	0x0017 },	//	54
	{	 1,	19,	 1,	 9,	0x0016 },	//	55
	{	 1,	20,	 1,	 9,	0x0015 },	//	56
	{	 1,	21,	 1,	 9,	0x0014 },	//	57
	{	 1,	22,	 1,	 9,	0x0013 },	//	58
	{	 1,	23,	 1,	 9,	0x0012 },	//	59
	{	 1,	24,	 1,	 9,	0x0011 },	//	60
	{	 0,	 0,	 8,	10,	0x0021 },	//	61
	{	 0,	 0,	 9,	10,	0x0020 },	//	62
	{	 0,	 1,	 4,	10,	0x000F },	//	63
	{	 0,	 2,	 3,	10,	0x000E },	//	64
	{	 0,	 3,	 3,	10,	0x000D },	//	65
	{	 0,	 5,	 2,	10,	0x000C },	//	66
	{	 0,	 6,	 2,	10,	0x000B },	//	67
	{	 0,	 7,	 2,	10,	0x000A },	//	68
	{	 0,	 8,	 2,	10,	0x0009 },	//	69
	{	 0,	 9,	 2,	10,	0x0008 },	//	70
	{	 1,	25,	 1,	10,	0x0007 },	//	71
	{	 1,	26,	 1,	10,	0x0006 },	//	72
	{	 1,	27,	 1,	10,	0x0005 },	//	73
	{	 1,	28,	 1,	10,	0x0004 },	//	74
	{	 0,	 0,	10,	11,	0x0007 },	//	75
	{	 0,	 0,	11,	11,	0x0006 },	//	76
	{	 0,	 0,	12,	11,	0x0020 },	//	77
	{	 0,	 1,	 5,	11,	0x0021 },	//	78
	{	 0,	23,	 1,	11,	0x0022 },	//	79
	{	 0,	24,	 1,	11,	0x0023 },	//	80
	{	 1,	 0,	 3,	11,	0x0005 },	//	81
	{	 1,	 1,	 2,	11,	0x0004 },	//	82
	{	 1,	29,	 1,	11,	0x0024 },	//	83
	{	 1,	30,	 1,	11,	0x0025 },	//	84
	{	 1,	31,	 1,	11,	0x0026 },	//	85
	{	 1,	32,	 1,	11,	0x0027 },	//	86
	{	 0,	 1,	 6,	12,	0x0050 },	//	87
	{	 0,	 2,	 4,	12,	0x0051 },	//	88
	{	 0,	 4,	 3,	12,	0x0052 },	//	89
	{	 0,	 5,	 3,	12,	0x0053 },	//	90
	{	 0,	 6,	 3,	12,	0x0054 },	//	91
	{	 0,	10,	 2,	12,	0x0055 },	//	92
	{	 0,	25,	 1,	12,	0x0056 },	//	93
	{	 0,	26,	 1,	12,	0x0057 },	//	94
	{	 1,	33,	 1,	12,	0x0058 },	//	95
	{	 1,	34,	 1,	12,	0x0059 },	//	96
	{	 1,	35,	 1,	12,	0x005A },	//	97
	{	 1,	36,	 1,	12,	0x005B },	//	98
	{	 1,	37,	 1,	12,	0x005C },	//	99
	{	 1,	38,	 1,	12,	0x005D },	//	100
	{	 1,	39,	 1,	12,	0x005E },	//	101
	{	 1,	40,	 1,	12,	0x005F }	//	102
};

