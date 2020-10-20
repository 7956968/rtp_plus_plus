/** @file

MODULE						: IntraLastRunLevelH263VlcEncoder

TAG								: ILRLH263VE

FILE NAME					: IntraLastRunLevelH263VlcEncoder.cpp

DESCRIPTION				: An Intra last-run-level Vlc encoder implementation as defined in
										H.263 Recommendation (02/98) Annex D Table I.2 page 75. This is
										defined as the Advanced Intra mode and is implemented with an
										IVlcEncoder Interface. The table has the same codes as those used
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

#include "IntraLastRunLevelH263VlcEncoder.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
#define ILRLH263VE_NUM_ESC_CODE_BITS			7			// ESC marker code.
#define ILRLH263VE_ESC_CODEWORD			 0x0003

// An ESC code is made up of the marker code, 1 bit for last, 6 bits
// for run, 8 bits for the level (and 11 bits for the extended level
// that is inserted with an ModQuantExtEscLevelH263VlcEncoder() object.
#define ILRLH263VE_NUM_ESC_LAST_BITS			1
#define ILRLH263VE_NUM_ESC_RUN_BITS				6
#define ILRLH263VE_NUM_ESC_LEVEL_BITS			8

// Short cuts into the VLC_TABLE.
#define ILRLH263VE_LAST_JUMP						 58

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
IntraLastRunLevelH263VlcEncoder::IntraLastRunLevelH263VlcEncoder(void)
{
	_numCodeBits	= 0;
	_bitCode			= 0;
}//end constructor.

IntraLastRunLevelH263VlcEncoder::~IntraLastRunLevelH263VlcEncoder(void)
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
members are set accordingly.
@param symbol1	: last
@param symbol2	: run
@param symbol3	: level
@return					: Num of bits in the codeword.
*/
int	IntraLastRunLevelH263VlcEncoder::Encode3(int symbol1, int symbol2, int symbol3)
{
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
		EncodeESC(last, run, symbol3);

	return(_numCodeBits);
}// end Encode3.


/*
---------------------------------------------------------------------------
	Private Methods.
---------------------------------------------------------------------------
*/
/** Encode ESC sequence last-run-level symbols.
The 3 param symbols represent last, run and level, respectively and are
encoded with the set escape sequence. The _numCodeBits and _bitCode
members are set accordingly.
@param symbol1	: last
@param symbol2	: run
@param symbol3	: level
@return					: none.
*/
void IntraLastRunLevelH263VlcEncoder::EncodeESC(int last, int run, int level)
{
	_bitCode = (ILRLH263VE_ESC_CODEWORD << 15);
	_bitCode |= (last << 14);
	_bitCode |= ((run & 0x3F) << 8);
	_bitCode |= (level & 0xFF);
	_numCodeBits = 22;
}//end EncodeESC.

/*
---------------------------------------------------------------------------
	Table Constant.
---------------------------------------------------------------------------
*/
// The table excludes the sign bit that is appended after selection.
const LastRunLevelType IntraLastRunLevelH263VlcEncoder::VLC_TABLE[ILRLH263VE_TABLE_LENGTH] = 
{//last run level bits code
	{	 0,	 0,	 1,	 2,	0x0002 },	//  0
	{	 0,	 1,	 1,	 4,	0x000F },	// 	1
	{	 0,	 3,	 1,	 6,	0x0015 },	// 	2
	{	 0,	 5,	 1,	 7,	0x0017 },	// 	3
	{	 0,	 7,	 1,	 8,	0x001F },	// 	4
	{	 0,	 8,	 1,	 9,	0x0025 },	// 	5
	{	 0,	 9,	 1,	 9,	0x0024 },	// 	6
	{	 0,	10,	 1,	10,	0x0021 },	// 	7
	{	 0,	11,	 1,	10,	0x0020 },	// 	8
	{	 0,	 4,	 3,	11,	0x0007 },	// 	9
	{	 0,	 9,	 2,	11,	0x0006 },	//  10
	{	 0,	13,	 1,	11,	0x0020 },	// 	11
	{	 0,	 0,	 2,	 3,	0x0006 },	// 	12
	{	 0,	 1,	 2,	 6,	0x0014 },	// 	13
	{	 0,	 1,	 4,	 8,	0x001E },	// 	14
	{	 0,	 1,	 5,	10,	0x000F },	// 	15
	{	 0,	 1,	 6,	11,	0x0021 },	// 	16
	{	 0,	 1,	 7,	12,	0x0050 },	// 	17
	{	 0,	 0,	 3,	 4,	0x000E },	// 	18
	{	 0,	 3,	 2,	 8,	0x001D },	// 	19
	{	 0,	 2,	 3,	10,	0x000E },	// 	20
	{	 0,	 3,	 4,	12,	0x0051 },	// 	21
	{	 0,	 0,	 5,	 5,	0x000D },	// 	22
	{	 0,	 4,	 2,	 9,	0x0023 },	// 	23
	{	 0,	 3,	 3,	10,	0x000D },	// 	24
	{	 0,	 0,	 4,	 5,	0x000C },	// 	25
	{	 0,	 5,	 2,	 9,	0x0022 },	// 	26
	{	 0,	 5,	 3,	12,	0x0052 },	// 	27
	{	 0,	 2,	 1,	 5,	0x000B },	//	28
	{	 0,	 6,	 2,	10,	0x000C },	//	29
	{	 0,	 0,	25,	12,	0x0053 },	//	30
	{	 0,	 4,	 1,	 6,	0x0013 },	//	31
	{	 0,	 7,	 2,	10,	0x000B },	//	32
	{	 0,	 0,	24,	12,	0x0054 },	//	33
	{	 0,	 0,	 8,	 6,	0x0012 },	//	34
	{	 0,	 8,	 2,	10,	0x000A },	//	35
	{	 0,	 0,	 7,	 6,	0x0011 },	//	36
	{	 0,	 2,	 4,	10,	0x0009 },	//	37
	{	 0,	 0,	 6,	 6,	0x0010 },	//	38
	{	 0,	12,	 1,	10,	0x0008 },	//	39
	{	 0,	 0,	 9,	 7,	0x0016 },	//	40
	{	 0,	 0,	23,	12,	0x0055 },	//	41
	{	 0,	 2,	 2,	 7,	0x0015 },	//	42
	{	 0,	 1,	 3,	 7,	0x0014 },	//	43
	{	 0,	 6,	 1,	 8,	0x001C },	//	44
	{	 0,	 0,	10,	 8,	0x001B },	//	45
	{	 0,	 0,	12,	 9,	0x0021 },	//	46
	{	 0,	 0,	11,	 9,	0x0020 },	//	47
	{	 0,	 0,	18,	 9,	0x001F },	//	48
	{	 0,	 0,	17,	 9,	0x001E },	//	49
	{	 0,	 0,	16,	 9,	0x001D },	//	50
	{	 0,	 0,	15,	 9,	0x001C },	//	51
	{	 0,	 0,	14,	 9,	0x001B },	//	52
	{	 0,	 0,	13,	 9,	0x001A },	//	53
	{	 0,	 0,	20,	11,	0x0022 },	//	54
	{	 0,	 0,	19,	11,	0x0023 },	//	55
	{	 0,	 0,	22,	12,	0x0056 },	//	56
	{	 0,	 0,	21,	12,	0x0057 },	//	57
	{	 1,	 0,	 1,	 4,	0x0007 },	//	58			Last = 1 jump position.
	{	 1,	14,	 1,	 9,	0x0019 },	//	59
	{	 1,	20,	 1,	11,	0x0005 },	//	60
	{	 1,	 1,	 1,	 6,	0x000F },	//	61
	{	 1,	19,	 1,	11,	0x0004 },	//	62
	{	 1,	 2,	 1,	 6,	0x000E },	//	63
	{	 1,	 3,	 1,	 6,	0x000D },	//	64
	{	 1,	 0,	 2,	 6,	0x000C },	//	65
	{	 1,	 5,	 1,	 7,	0x0013 },	//	66
	{	 1,	 6,	 1,	 7,	0x0012 },	//	67
	{	 1,	 4,	 1,	 7,	0x0011 },	//	68
	{	 1,	 0,	 3,	 7,	0x0010 },	//	69
	{	 1,	 9,	 1,	 8,	0x001A },	//	70
	{	 1,	10,	 1,	 8,	0x0019 },	//	71
	{	 1,	11,	 1,	 8,	0x0018 },	//	72
	{	 1,	12,	 1,	 8,	0x0017 },	//	73
	{	 1,	13,	 1,	 8,	0x0016 },	//	74
	{	 1,	 8,	 1,	 8,	0x0015 },	//	75
	{	 1,	 7,	 1,	 8,	0x0014 },	//	76
	{	 1,	 0,	 4,	 8,	0x0013 },	//	77
	{	 1,	17,	 1,	 9,	0x0018 },	//	78
	{	 1,	18,	 1,	 9,	0x0017 },	//	79
	{	 1,	16,	 1,	 9,	0x0016 },	//	80
	{	 1,	15,	 1,	 9,	0x0015 },	//	81
	{	 1,	 2,	 2,	 9,	0x0014 },	//	82
	{	 1,	 1,	 2,	 9,	0x0013 },	//	83
	{	 1,	 0,	 6,	 9,	0x0012 },	//	84
	{	 1,	 0,	 5,	 9,	0x0011 },	//	85
	{	 1,	 4,	 2,	10,	0x0007 },	//	86
	{	 1,	 3,	 2,	10,	0x0006 },	//	87
	{	 1,	 1,	 2,	10,	0x0005 },	//	88
	{	 1,	 0,	 7,	10,	0x0004 },	//	89
	{	 1,	 2,	 3,	11,	0x0024 },	//	90
	{	 1,	 1,	 4,	11,	0x0025 },	//	91
	{	 1,	 0,	 9,	11,	0x0026 },	//	92
	{	 1,	 0,	 8,	11,	0x0027 },	//	93
	{	 1,	21,	 1,	12,	0x0058 },	//	94
	{	 1,	22,	 1,	12,	0x0059 },	//	95
	{	 1,	23,	 1,	12,	0x005A },	//	96
	{	 1,	 7,	 2,	12,	0x005B },	//	97
	{	 1,	 6,	 2,	12,	0x005C },	//	98
	{	 1,	 5,	 2,	12,	0x005D },	//	99
	{	 1,	 3,	 3,	12,	0x005E },	//	100
	{	 1,	 0,	10,	12,	0x005F }	//	101
};

