/** @file

MODULE						: RunLengthVlcDecoder

TAG								: RLVD

FILE NAME					: RunLengthVlcDecoder.cpp

DESCRIPTION				: A class to implement a run length vlc decoder derived
										from an IVlcDecoder interface.

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

#include "RunLengthVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
const int RunLengthVlcDecoder::NUM_ESC_CODE_BITS	= 5;
const int RunLengthVlcDecoder::ESC_BIT_CODE				= 0x0018;
const int RunLengthVlcDecoder::EOP_MARKER					= 32;
const int RunLengthVlcDecoder::EOI_MARKER					= 33;

RunLengthVlcDecoder::RunLengthVlcDecoder()
{
	_numCodeBits	= 0;
	_marker				= 0;
	_numEscBits		= 8;
	_escMask			= 255;
}//end constructor.

RunLengthVlcDecoder::~RunLengthVlcDecoder()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
int RunLengthVlcDecoder::Decode(IBitStreamReader* bsr)
{
  int bit;
  int bits				= 0;
  int tblPos			= 0;
	int tblSize			= RLVD_TABLE_SIZE + 2; // Include markers.;
  int found				= 0;
  int bitsSoFar		= 0;
	int bitsNeeded	= 0;
	_marker = 0; // Default.

  // Decode the binary tree to determine the run/level.
  // The table must be in ascending bit length order. Bits 
  // are extracted from the bit stream depending on the next
  // no. of bits in the table. Keep going through the table
  // until a match is found or the table ends.
  while( (tblPos < tblSize) && !found )
  {
    bitsNeeded = VLC_TABLE[tblPos][RLVD_NUM_BITS] - bitsSoFar;
    // Get the bits off the bit stream.
		bit  = bsr->Read(bitsNeeded);
    bits = bits | (bit << bitsSoFar);
    bitsSoFar += bitsNeeded;

    // Advance down the table checking the codes with the current 
		// no. of bits so far.
    while( (VLC_TABLE[tblPos][RLVD_NUM_BITS] == bitsSoFar) && 
           (tblPos < tblSize) && !found )
    {
      if(VLC_TABLE[tblPos][RLVD_BIT_CODE] == bits)
        found = 1;
      else
        tblPos++;

    }//end while VLC_TABLE...

    // Check for escape sequence at the appropriate numbits pos.
    if(!found && (bitsSoFar == NUM_ESC_CODE_BITS))
    {
      if(bits == ESC_BIT_CODE)
      {
				int run = bsr->Read(_numEscBits);
        _numCodeBits = NUM_ESC_CODE_BITS + _numEscBits;
        return(run);
      }//end if bits...
    }//end if !found...
  }//end while tblPos...

  // If not found then there is an error.
  if( !found )
  {
    _numCodeBits	= 0; // Implies an error. 
    return(0);
  }//end if !found...

	// Check for marker codes.
  if((tblPos == EOP_MARKER)||(tblPos == EOI_MARKER))
    _marker = 1;

  _numCodeBits = bitsSoFar;
  return(tblPos);
}//end Decode.

/*
---------------------------------------------------------------------------
	Constant vlc table.
---------------------------------------------------------------------------
*/
const int RunLengthVlcDecoder::VLC_TABLE[RLVD_TABLE_SIZE+3][2] =
{																		// run
	{  1 					,	0x0001			 },		//	 0   
	{  3 					, 0x0002			 },		//	 1
	{  3 					, 0x0006			 },  	//   2
	{  4 					, 0x0004			 },		//	 3
	{  4 					, 0x000C			 },  	//   4
	{  5 					, 0x0008			 },		//	 5
	{  7 					, 0x0030			 },  	//   6
	{  7 					, 0x0070			 },		//   7
	{  8 					, 0x0050			 },  	//	 8
	{  8 					, 0x00D0			 },		//	 9
	{  8 					, 0x0010			 },  	//  10
	{  8 					, 0x0090			 },		//  11
	{  8 					, 0x0060			 },  	//  12	
	{  8 					, 0x00E0			 },		//  13
	{ 10 					, 0x01A0			 },  	//  14
	{ 10 					, 0x03A0			 },		//  15
	{ 10 					, 0x00A0			 },  	//  16
	{ 10 					, 0x02A0			 },		//  17
	{ 10 					, 0x0120			 },  	//  18
	{ 10 					, 0x0320			 },		//	19
	{ 11 					, 0x0220			 },  	//	20
	{ 11 					, 0x0620			 },		//	21
	{ 11 					, 0x0020			 },  	//	22
	{ 11 					, 0x0420			 },		//	23
	{ 11 					, 0x03C0			 },  	//	24
	{ 11 					, 0x07C0			 },		//	25
	{ 11 					, 0x01C0			 },  	//	26
	{ 11 					, 0x05C0			 },		//	27
	{ 11 					, 0x02C0			 },  	//  28
	{ 11 					, 0x06C0			 },		//	29
	{ 11 					, 0x00C0			 },  	//	30
	{ 11 					, 0x04C0			 },		//	31
	{ 11					, 0x0340       },   //  32	Marker codes:
	{ 11					, 0x0740       },   //  33
	{  5					, 0x0018			 }		//	Esc code.
};

