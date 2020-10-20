/** @file

MODULE						: MotionVectorVlcDecoder

TAG								: MVVD

FILE NAME					: MotionVectorVlcDecoder.cpp

DESCRIPTION				: A motion vector Vlc decoder implementation with an
										IVlcDecoder Interface.

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

#include "MotionVectorVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
const int MotionVectorVlcDecoder::NUM_ESC_BITS	= 5;
const int MotionVectorVlcDecoder::ESC_BIT_CODE	= 0x000A;
const int MotionVectorVlcDecoder::ESC_LENGTH		= 8;

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
int MotionVectorVlcDecoder::Decode(IBitStreamReader* bsr)
{
  // Decode the binary tree to determine the motion vector coord.
  // The table must be in ascending bit length order. Bits
  // are extracted from the bit stream depending on the next
  // no. of bits in the table. Keep going through the table
  // until a match is found or the table ends.
  int bits				= 0;
  int tblPos			= 0;
  int bitsSoFar		= 0;
  int bitsNeeded	= 0;
  int found				= 0;
  
  while( (tblPos < MVVD_TABLE_SIZE) && !found )
  {
		bitsNeeded = VLC_TABLE[tblPos][MVVD_NUM_BITS] - bitsSoFar;
    // Get the bits off the bit stream.
		int bit = bsr->Read(bitsNeeded);
    bits = bits | (bit << bitsSoFar);
    bitsSoFar += bitsNeeded;

    // Advance down the table checking the codes with the current no. of bits so far.
    while( (VLC_TABLE[tblPos][MVVD_NUM_BITS] == bitsSoFar) && 
           (tblPos < MVVD_TABLE_SIZE) && !found )
    {
      if(bits == VLC_TABLE[tblPos][MVVD_BIT_CODE])
        found = 1;
			else if((bits == ESC_BIT_CODE)&&(bitsSoFar == NUM_ESC_BITS))
			{
				bit = bsr->Read(ESC_LENGTH);
				bitsSoFar += ESC_LENGTH;
				found = 1;
				tblPos = bit - 128;
			}//end else if bits...
      else
        tblPos++;
    }//end while numbits...
  }//end while tblPos...

  // If not found then there is an error.
  if( !found )
  {
    _numDecodedBits	= 0; // Implies an error. 
    return(0);
  }//end if !Found...

	if(bits != ESC_BIT_CODE)
	{
		// Get the signed codeword from the table.
		int sgn = tblPos % 2;
		tblPos = tblPos / 2;

		if (sgn == 1)
			++tblPos;
		else
			tblPos = -tblPos;
	}//end if bits...

  _numDecodedBits = bitsSoFar;
  return(tblPos);
}//end Decode.

int	MotionVectorVlcDecoder::Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2)
{
	*symbol1	= Decode(bsr);
	int xBits = _numDecodedBits;
	if(!xBits)
	{
		_numDecodedBits = 0;
		return(0);
	}//end if !xBits...

	*symbol2	= Decode(bsr);
	int yBits = _numDecodedBits;
	if(!yBits)
	{
		_numDecodedBits = 0;
		return(0);
	}//end if !yBits...

	return(xBits + yBits);
}//end Decode2.

/*
---------------------------------------------------------------------------
	Constant motion vector table.
---------------------------------------------------------------------------
*/
const int MotionVectorVlcDecoder::VLC_TABLE[MVVD_TABLE_SIZE][2] =
{
	{ 2 					,	0x0001			 },		//	0   
	{ 3 					, 0x0006			 },		//	1
	{ 3 					, 0x0004			 },  	// -1
	{ 4 					, 0x0000			 },		//	2
	{ 4 					, 0x000b			 },  	// -2
	{ 5 					, 0x0018			 },		//	3
	{ 5 					, 0x0013			 },  	// -3
	{ 6 					, 0x003a			 },		//	4
	{ 6 					, 0x001a			 },  
	{ 6 					, 0x0012			 },		//	5
	{ 6 					, 0x0002			 },  
	{ 6 					, 0x0008			 },		//	6
	{ 6 					, 0x0023			 },  
	{ 6 					, 0x0003			 },		//	7
	{ 6 					, 0x001f			 },  
	{ 6 					, 0x0027			 },		//	8
	{ 7 					, 0x0032			 },  
	{ 7 					, 0x0062			 },		//	9
	{ 7 					, 0x0068			 },  
	{ 7 					, 0x0047			 },		//	10
	{ 8 					, 0x00f2			 },  
	{ 8 					, 0x0072			 },		//	11
	{ 8 					, 0x0022			 },  
	{ 8 					, 0x00a8			 },		//	12
	{ 8 					, 0x0028			 },  
	{ 8 					, 0x0097			 },		//	13
	{ 8 					, 0x0087			 },  
	{ 8 					, 0x0017			 },		//	14
	{ 8 					, 0x0007			 },  
	{ 9 					, 0x01a2			 },		//	15
	{ 9 					, 0x00a2			 },  
	{ 9 					, 0x00ff			 },		//	16
	{ 9 					, 0x0057			 },  
	{ 9 					, 0x0157			 },		//	17
	{ 9 					, 0x00d7			 },  
	{ 9 					, 0x01d7			 },		//	18
	{ 9 					, 0x0037			 },  
	{ 9 					, 0x0137			 },		//	19
	{ 9 					, 0x00b7			 },  
	{ 9 					, 0x01b7			 },		//	20
	{ 9 					, 0x0077			 },  
	{ 9 					, 0x0177			 },		//	21
	{ 9 					, 0x00f7			 },  
	{ 9 					, 0x01f7			 },		//	22
	{ 9 					, 0x000f			 },  
	{ 9 					, 0x010f			 },		//	23
	{ 9 					, 0x008f			 },  
	{ 9 					, 0x018f			 },		//	24
	{ 9 					, 0x004f			 },  
	{ 9 					, 0x014f			 },		//	25
	{ 9 					, 0x00cf			 },  
	{ 9 					, 0x01cf			 },		//	26
	{ 9 					, 0x002f			 },  
	{ 9 					, 0x012f			 },		//	27
	{ 9 					, 0x00af			 },  
	{ 9 					, 0x01af			 },		//	28
	{ 9 					, 0x006f			 },  
	{ 9 					, 0x016f			 },		//	29
	{ 9 					, 0x00ef			 },  
	{ 9 					, 0x01ef			 },		//	30
	{ 9 					, 0x003f			 },  
	{ 9 					, 0x013f			 },		//	31
	{ 9 					, 0x00bf			 },  
	{ 9 					, 0x01bf			 },		//	32
	{ 9						,	0x007f			 },
	{ NUM_ESC_BITS, ESC_BIT_CODE }
};
