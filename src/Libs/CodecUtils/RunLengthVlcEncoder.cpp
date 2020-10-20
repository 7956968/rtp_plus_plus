/** @file

MODULE						: RunLengthVlcEncoder

TAG								: RLVE

FILE NAME					: RunLengthVlcEncoder.cpp

DESCRIPTION				: A class to implement a run length vlc encoder derived
										from an IVlcEncoder interface.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2005  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include "RunLengthVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
const int RunLengthVlcEncoder::NUM_ESC_CODE_BITS	= 5;
const int RunLengthVlcEncoder::ESC_BIT_CODE				= 0x0018;

const int RunLengthVlcEncoder::VLC_TABLE[RLVE_TABLE_SIZE][2] =
{
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
	{ 11 					, 0x04C0			 }		//	31
};

/*
---------------------------------------------------------------------------
	Private Methods.
---------------------------------------------------------------------------
*/
int RunLengthVlcEncoder::DoEncode(int		run,
																	int		runEscBits,
																	int		runEscMask,
																	int*	codeWord)
{
  if(run < 0)
  {
    *codeWord = 0;
    return(0);
  }//end if run...
  if(run < RLVE_TABLE_SIZE)
  {
    *codeWord = VLC_TABLE[run][RLVE_BIT_CODE];
    return(VLC_TABLE[run][RLVE_NUM_BITS]);
  }//end if run...
  else
  {
    *codeWord = (((run & runEscMask) << NUM_ESC_CODE_BITS) | ESC_BIT_CODE);
    return(NUM_ESC_CODE_BITS + runEscBits);
  }//end else...

}//end DoEncode.

